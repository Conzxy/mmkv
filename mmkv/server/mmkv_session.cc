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
      db::DataType type;
      if (DB.Type(request->key, type)) {
        SET_OK_VALUE(GetDataTypeString(type));
      } else {
        response.status_code = S_NONEXISTS;
      }
    }
      break;

    case DEL: {
      SET_OK_ELSE_NONEXISTS(DB.Delete(request->key));
    }
      break;

    case RENAME: {
      auto code = DB.Rename(request->key, std::move(request->value));
      response.status_code = code;
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
      // if (code == S_OK) {
      //   response.SetOk();
      //   response.SetValue();
      //   response.value = *str;
      // } else {
      //   response.status_code = code;
      // }
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
      
      if (code) {
        *str = std::move(request->value);
        response.SetOk();
      } else {
        response.status_code = code;
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
      // switch (status_code) {
      //   case S_OK:
      //     response.SetOk();
      //     response.SetValues();
      //     response.values = std::move(values);
      //     break;
      //   default:
      //     response.status_code = status_code;
      // }
    }
      break;
    case LGETALL: {
      MMKV_ASSERT(request->HasKey(), "lgetrange/getall");
      
      StrValues values; 
      auto code = DB.ListGetAll(request->key, values);
      SET_XX_ELSE_CODE(SET_OK_VALUES(std::move(values)))

      // if (!success) {
      //   response.status_code = S_NONEXISTS;
      // } else {
      //   response.SetOk();
      //   response.SetValues();
      //   response.values = std::move(values);
      // }
    }
      break;
    case LDEL: {
      response.status_code = DB.ListDel(request->key);
    }
      break;
    case VADD: {
      size_t count = 0;
      auto code = DB.VsetAdd(std::move(request->key), std::move(request->vmembers), count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      // if (code == S_OK) {
      //   response.SetOk(); 
      //   response.SetCount();
      //   response.count = count;
      // } else {
      //   response.status_code = code;
      // }
    }
      break;

    case VALL: {
      WeightValues values;
      auto code = DB.VsetAll(request->key, values);
      SET_XX_ELSE_CODE(SET_OK_VMEMBERS(std::move(values)))
      // if (code == S_OK) {
      //   response.SetOk();
      //   response.SetVmembers();
      //   response.vmembers = std::move(*values);
      // } else {
      //   response.status_code = S_NONEXISTS;
      // }
    }
      break;
    
    case VDELM: {
      response.status_code = DB.VsetDel(request->key, request->value);
    }
      break;
    
    case VDELMRANGE: {
      size_t count = 0;
      auto code = DB.VsetDelRange(request->key, request->range, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      // if (code == S_OK) {
      //   response.SetOk();
      //   response.SetCount();
      //   response.count = count;
      // } else {
      //   response.status_code = code;
      // }
    }
      break;

    case VDELMRANGEBYWEIGHT: {
      size_t count = 0;
      auto code = DB.VsetDelRangeByWeight(request->key, request->GetWeightRange(), count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      // if (code == S_OK) {
      //   response.SetOk();
      //   response.SetCount();
      //   response.count = count;
      // } else {
      //   response.status_code = code;
      // }
    }
      break;
    
    case VSIZE:
    case VSIZEBYWEIGHT: {
      size_t size;
      auto code = request->command == VSIZE ? 
        DB.VsetSize(request->key, size) :
        DB.VsetSizeByWeight(request->key, request->GetWeightRange(), size);
      SET_XX_ELSE_CODE(SET_OK_COUNT(size))
      // if (code == S_OK) {
      //   response.SetOk();
      //   response.SetCount();
      //   response.count = size;
      // } else {
      //   response.status_code = code;
      // }
    }
      break;
    
    case VWEIGHT: {
      Weight weight;
      auto code = DB.VsetWeight(request->key, request->value, weight);
      SET_XX_ELSE_CODE(SET_OK_COUNT(util::double2u64(weight)))
      // switch (code) {
      //   case S_OK:
      //     response.status_code = S_OK;
      //     response.SetCount();
      //     response.count = util::double2u64(weight);
      //     break;
      //   default:
      //     response.status_code = code;
      // }
    }
      break;

    case VORDER:
    case VRORDER: {
      size_t order;
      auto code = request->command == VORDER ? 
        DB.VsetOrder(request->key, request->value, order) :
        DB.VsetROrder(request->key, request->value, order);
      SET_XX_ELSE_CODE(SET_OK_COUNT(order))
      // switch (code) {
      //   case S_OK:
      //     response.status_code = S_OK;
      //     response.SetCount();
      //     response.count = order;
      //     break;
      //   default:
      //     response.status_code = code;
      // }      
    }
      break;

    case VRANGE:
    case VRRANGE: {
      WeightValues wms;
      auto code = request->command == VRANGE ?
        DB.VsetRange(request->key, request->range, wms) :
        DB.VsetRRange(request->key, request->range, wms);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS(std::move(wms)))
      // if (code == S_OK) {
      //   response.SetOk();
      //   response.SetVmembers();
      //   response.vmembers = std::move(wms);
      // } else {
      //   response.status_code = S_NONEXISTS;
      // }
    }
      break;

    case VRANGEBYWEIGHT:
    case VRRANGEBYWEIGHT: {
      WeightValues wms;
      auto code = request->command == VRANGEBYWEIGHT ?
        DB.VsetRangeByWeight(request->key, request->GetWeightRange(), wms) :
        DB.VsetRRangeByWeight(request->key, request->GetWeightRange(), wms);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS(std::move(wms)))
      // if (code == S_OK) {
      //   response.SetOk();
      //   response.SetVmembers();
      //   response.vmembers = std::move(wms);
      // } else {
      //   response.status_code = S_NONEXISTS;
      // }
    }
      break;
  }
  
  response.DebugPrint();  
  codec_.Send(conn, &response);
}