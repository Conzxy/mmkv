#include "mmkv_session.h"

#include "mmkv/algo/hash_util.h"
#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/protocol/status_code.h"
#include "mmkv/util/macro.h"
#include "mmkv/util/memory_footprint.h"
#include "mmkv_server.h"

#include <kanon/net/callback.h>
#include <kanon/log/logger.h>
#include <kanon/util/ptr.h>

using namespace kanon;
using namespace mmkv::protocol;
using namespace mmkv::server;

#define DB server_->db_

MmkvSession::MmkvSession(TcpConnectionPtr const& conn, MmkvServer* server)
  : conn_(conn.get())
  , codec_(MmbpRequest::GetPrototype())
  , server_(server)
{
  codec_.SetUpConnection(conn);
  codec_.SetMessageCallback(std::bind(&MmkvSession::OnMmbpRequest, this, _1, _2, _3));
}

MmkvSession::~MmkvSession() noexcept {

}

#define SET_OK_ELSE_(condition, code) do { \
  if ((condition)) { \
    response.SetOk(); \
  } else { \
    response.status_code = (code); \
  } } while (0)

#define SET_OK_ELSE_NONEXISTS(condition) \
  SET_OK_ELSE_(condition, S_NONEXISTS)

#define SET_OK_ELSE_EXISTS(condition) \
  SET_OK_ELSE_(condition, S_EXISTS)

#define SET_OK_VALUE(_value) \
  response.SetOk(); \
  response.SetValue(); \
  response.value = (_value)

#define SET_OK_VALUES(_values) \
  response.SetOk(); \
  response.SetValues(); \
  response.values = (_values)

#define SET_OK_VMEMBERS(_wms) \
  response.SetOk(); \
  response.SetVmembers(); \
  response.vmembers = (_wms)

#define SET_OK_KVS(_kvs) \
  response.SetOk(); \
  response.SetKvs(); \
  response.kvs = (_kvs);

#define SET_OK_COUNT(_count) \
  response.SetOk(); \
  response.SetCount(); \
  response.count = (_count)

#define SET_OK_COUNT_ \
  response.SetOk(); \
  response.SetCount(); \

#define SET_OK \
  response.SetOk()

#define SET_XX_ELSE_CODE(xx) \
  if (code == S_OK) { \
    xx; \
  } else { \
    response.status_code = code; \
  }

void MmkvSession::OnMmbpRequest(TcpConnectionPtr const& conn, std::unique_ptr<MmbpMessage> message, TimeStamp recv_time) {
  MMKV_UNUSED(recv_time);

  auto request = kanon::down_pointer_cast<MmbpRequest>(message);
  request->DebugPrint();

  MmbpResponse response; 
  
  switch (request->command) {
    case MEM_STAT: {
      MMKV_ASSERT(request->HasNone(), "memorystat");
      SET_OK_VALUE(util::GetMemoryStat());
    }
      break;

    case TYPE: {
      MMKV_ASSERT(request->HasKey(), "type");
      db::DataType type;
      if (DB.Type(request->key, type)) {
        SET_OK_VALUE(GetDataTypeString(type));
      } else {
        response.status_code = S_NONEXISTS;
      }
    }
      break;

    case DEL: {
      MMKV_ASSERT(request->HasKey(), "del");
      SET_OK_ELSE_NONEXISTS(DB.Delete(request->key));
    }
      break;

    case RENAME: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "rename");
      auto code = DB.Rename(request->key, std::move(request->value));
      response.status_code = code;
    }
      break;

    case KEYALL: {
      DB.GetAllKeys(response.values);
      response.SetValues();
      response.SetOk();
    }
      break;

    case STR_ADD: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "stradd");
      
      auto code = DB.InsertStr(std::move(request->key), std::move(request->value));
      SET_XX_ELSE_CODE(SET_OK)
    }
      break;
    case STR_GET: {
      MMKV_ASSERT(request->HasKey(), "strget");

      String* str = nullptr;
      auto code = DB.GetStr(request->key, str);
      SET_XX_ELSE_CODE(SET_OK_VALUE(*str))
    }
      break;
    case STR_DEL: {
      MMKV_ASSERT(request->HasKey(), "strdel");

      auto code = DB.EraseStr(request->key);
      response.status_code = code;
    }
      break;

    case STR_SET: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "strset");

      String* str = nullptr;
      const auto code = DB.GetStr(request->key, str);
      
      if (code == S_OK) {
        *str = std::move(request->value);
        response.SetOk();
      } else {
        response.status_code = code;
      }
    }
      break;

    case STRLEN: {
      MMKV_ASSERT(request->HasKey(), "strlen");
      String* str = nullptr;
      const auto code = DB.GetStr(request->key, str);

      response.status_code = code;
      if (code == S_OK) {
        response.count = str->size();
        response.SetCount();
      }
    }
      break;

    case STRAPPEND: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "strappend");
      String* str = nullptr;
      const auto code = DB.GetStr(request->key, str);

      response.status_code = code;
      if (code == S_OK) {
        str->append(request->value);
      }
    }
      break;

    case STRPOPBACK: {
      MMKV_ASSERT(request->HasKey() && request->HasCount(), "strpopback");
      String* str = nullptr;
      const auto code = DB.GetStr(request->key, str);

      response.status_code = code;
      if (code == S_OK) {
        str->erase(str->size()-request->count, request->count);
      }
    }
      break;

    case LADD: {
      MMKV_ASSERT(request->HasKey() && request->HasValues(), "ladd");
      auto code = DB.ListAdd(std::move(request->key), request->values);
      SET_XX_ELSE_CODE(SET_OK)
    }
      break;

    case LAPPEND:
    case LPREPEND: {
      MMKV_ASSERT(request->HasKey() && request->HasValues(), "lappend/lprepend");

      StatusCode code; 
      switch (request->command) {
        case LAPPEND:
          code = DB.ListAppend(request->key, request->values);
          break;
        case LPREPEND:
          code = DB.ListPrepend(request->key, request->values);
          break;
      }
      SET_XX_ELSE_CODE(SET_OK)
    }
      break;
    
    case LPOPFRONT:
    case LPOPBACK: {
      MMKV_ASSERT(request->HasKey() && request->HasCount(), "lpopback/popfront");

      StatusCode code;
      switch (request->command) {
        case LPOPFRONT:
          code = DB.ListPopFront(request->key, request->count);
          break;
        case LPOPBACK:
          code = DB.ListPopBack(request->key, request->count);
          break;
      }

      SET_XX_ELSE_CODE(SET_OK_COUNT_)
    }
      break;

    case LGETSIZE: {
      MMKV_ASSERT(request->HasKey(), "lgetsize");
      size_t count = 0;
      const auto code = DB.ListGetSize(request->key, count);

      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
    }
      break;

    case LGETRANGE: {
      MMKV_ASSERT(request->HasRange(), "lgetrange");

      StrValues values;
      auto code = DB.ListGetRange(request->key, values, request->range.left, request->range.right);

      SET_XX_ELSE_CODE(SET_OK_VALUES(std::move(values)))
    }
      break;

    case LGETALL: {
      MMKV_ASSERT(request->HasKey(), "lgetrange/getall");
      
      StrValues values; 
      auto code = DB.ListGetAll(request->key, values);
      SET_XX_ELSE_CODE(SET_OK_VALUES(std::move(values)))
    }
      break;

    case LDEL: {
      MMKV_ASSERT(request->HasKey(), "ldel");
      response.status_code = DB.ListDel(request->key);
    }
      break;

    case VADD: {
      MMKV_ASSERT(request->HasKey() && request->HasVmembers(), "vadd");
      size_t count = 0;
      auto code = DB.VsetAdd(std::move(request->key), std::move(request->vmembers), count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
    }
      break;

    case VALL: {
      MMKV_ASSERT(request->HasKey(), "vall");
      WeightValues values;
      auto code = DB.VsetAll(request->key, values);
      SET_XX_ELSE_CODE(SET_OK_VMEMBERS(std::move(values)))
    }
      break;
    
    case VDELM: {
      MMKV_ASSERT(request->HasKey(), "vdelm");
      response.status_code = DB.VsetDel(request->key, request->value);
    }
      break;
    
    case VDELMRANGE: {
      MMKV_ASSERT(request->HasKey() && request->HasRange(), "vdelmrange");
      size_t count = 0;
      auto code = DB.VsetDelRange(request->key, request->range, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
    }
      break;

    case VDELMRANGEBYWEIGHT: {
      MMKV_ASSERT(request->HasKey() && request->HasRange(), "vdelmrangebyweight");
      size_t count = 0;
      auto code = DB.VsetDelRangeByWeight(request->key, request->GetWeightRange(), count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
    }
      break;
    
    case VSIZE: {
      MMKV_ASSERT(request->HasKey(), "vsize");
      size_t size;
      auto code = DB.VsetSize(request->key, size);
      SET_XX_ELSE_CODE(SET_OK_COUNT(size))
    }
      break;

    case VSIZEBYWEIGHT: {
      MMKV_ASSERT(request->HasKey() && request->HasRange(), "vsizebyweight");
      size_t size;
      auto code = DB.VsetSizeByWeight(request->key, request->GetWeightRange(), size);
      SET_XX_ELSE_CODE(SET_OK_COUNT(size))
    }
      break;
    
    case VWEIGHT: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "vweight");
      Weight weight;
      auto code = DB.VsetWeight(request->key, request->value, weight);
      SET_XX_ELSE_CODE(SET_OK_COUNT(util::double2u64(weight)))
    }
      break;

    case VORDER: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "vorder");
      size_t order = 0;
      auto code = DB.VsetOrder(request->key, request->value, order);
      SET_XX_ELSE_CODE(SET_OK_COUNT(order))
    }
      break;

    case VRORDER: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "vrorder");
      size_t order = 0;
      auto code = DB.VsetROrder(request->key, request->value, order);
      SET_XX_ELSE_CODE(SET_OK_COUNT(order))
    }
      break;

    case VRANGE: {
      MMKV_ASSERT(request->HasKey() && request->HasRange(), "vrange");
      WeightValues wms;
      auto code = DB.VsetRange(request->key, request->range, wms);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS(std::move(wms)))

    }
      break;

    case VRRANGE: {
      MMKV_ASSERT(request->HasKey() && request->HasRange(), "vrrange");
      WeightValues wms;
      auto code = DB.VsetRRange(request->key, request->range, wms);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS(std::move(wms)))
    }
      break;

    case VRANGEBYWEIGHT: {
      MMKV_ASSERT(request->HasKey() && request->HasRange(), "vrangebyweight");
      WeightValues wms;
      auto code = DB.VsetRangeByWeight(request->key, request->GetWeightRange(), wms);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS(std::move(wms)))     
    }
      break;

    case VRRANGEBYWEIGHT: {
      MMKV_ASSERT(request->HasKey() && request->HasRange(), "vrrangebyweight");
      WeightValues wms;
      auto code = DB.VsetRRangeByWeight(request->key, request->GetWeightRange(), wms);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS(std::move(wms)))
    }
      break;

    case MADD: {
      MMKV_ASSERT(request->HasKey() && request->HasKvs(), "madd");
      size_t count = 0;
      auto code = DB.MapAdd(std::move(request->key), std::move(request->kvs), count);

      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    }
      break;

    case MGET: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "mget");
      String value;
      auto code = DB.MapGet(request->key, request->value, value);
      SET_XX_ELSE_CODE(SET_OK_VALUE(std::move(value)));
    }
      break;

    case MGETS: {
      MMKV_ASSERT(request->HasKey() && request->HasValues(), "mgets");
      StrValues values;
      auto code = DB.MapGets(request->key, request->values, values);
      SET_XX_ELSE_CODE(SET_OK_VALUES(std::move(values)));
    }

    case MSET: {
      MMKV_ASSERT(request->HasKey() && request->HasValues(), "mset");
      auto code = DB.MapSet(request->key, std::move(request->values[0]), std::move(request->values[1]));
      response.status_code = code;
    }
      break;

    case MDEL: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "mdel");
      response.status_code = DB.MapDel(request->key, request->value);
    }
      break;

    case MALL: {
      MMKV_ASSERT(request->HasKey(), "mall");
      StrKvs kvs;
      auto code = DB.MapAll(request->key, kvs);

      SET_XX_ELSE_CODE(SET_OK_KVS(std::move(kvs)));
    }
      break;

    case MFIELDS: {
      MMKV_ASSERT(request->HasKey(), "mfields");
      StrValues values;
      auto code = DB.MapFields(request->key, values);
      SET_XX_ELSE_CODE(SET_OK_VALUES(std::move(values)));
    }
      break;

    case MVALUES: {
      MMKV_ASSERT(request->HasKey(), "mvalues");
      StrValues values;
      auto code = DB.MapValues(request->key, values);
      SET_XX_ELSE_CODE(SET_OK_VALUES(std::move(values)));
    }
      break;

    case MSIZE: {
      MMKV_ASSERT(request->HasKey(), "msize");
      size_t count = 0;
      auto code = DB.MapSize(request->key, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    }
      break;

    case MEXISTS: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "mexists");
      response.status_code = DB.MapExists(request->key, request->value);
    }
      break;
  
    case SADD: {
      MMKV_ASSERT(request->HasKey() && request->HasValues(), "sadd");
      response.status_code = DB.SetAdd(std::move(request->key), request->values, response.count);
      if (response.status_code == S_OK)
        response.SetCount();
    }
      break;
    
    case SDELM: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "sdelm");
      response.status_code = DB.SetDelm(request->key, request->value);
    }
      break;

    case SEXISTS: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "sexists");
      response.status_code = DB.SetExists(request->key, request->value);
    }
      break;
    
    case SSIZE: {
      MMKV_ASSERT(request->HasKey(), "ssize");
      size_t count = 0;
      response.status_code = DB.SetSize(request->key, count);
      if (response.status_code == S_OK) {
        response.count = count;
        response.SetCount();
      }
    }
      break;
    
    case SALL: {
      MMKV_ASSERT(request->HasKey(), "sall");
      response.status_code = DB.SetAll(request->key, response.values);
      if (response.status_code == S_OK)
        response.SetValues();
    }
      break;
    
    case SAND: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "sand");
      response.status_code = DB.SetAnd(request->key, request->value, response.values);
      if (response.status_code == S_OK)
        response.SetValues();
    }
      break;

    case SOR: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "sor");
      response.status_code = DB.SetOr(request->key, request->value, response.values);
      if (response.status_code == S_OK)
        response.SetValues();
    }
      break;

    case SSUB: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "ssub");
      response.status_code = DB.SetSub(request->key, request->value, response.values);
      if (response.status_code == S_OK)
        response.SetValues();
    }
      break;

    case SANDTO: {
      MMKV_ASSERT(request->HasValues(), "sandto");
      response.status_code = DB.SetAndTo(request->values[1], request->values[2], std::move(request->values[0]));
    }
      break;

    case SORTO: {
      MMKV_ASSERT(request->HasValues(), "sorto");
      response.status_code = DB.SetOrTo(request->values[1], request->values[2], std::move(request->values[0]));
    }
      break;

    case SSUBTO: {
      MMKV_ASSERT(request->HasValues(), "ssubto");
      response.status_code = DB.SetSubTo(request->values[1], request->values[2], std::move(request->values[0]));
    }
      break;

    case SANDSIZE: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "sandsize");
      size_t count = 0;
      auto code = DB.SetAndSize(request->key, request->value, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    }
      break;

    case SORSIZE: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "sorsize");
      size_t count = 0;
      auto code = DB.SetOrSize(request->key, request->value, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    }
      break;

    case SSUBSIZE: {
      MMKV_ASSERT(request->HasKey() && request->HasValue(), "ssubsize");
      size_t count = 0;
      auto code = DB.SetSubSize(request->key, request->value, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    }
      break;
  }

  
  response.DebugPrint();  
  codec_.Send(conn, &response);
}
