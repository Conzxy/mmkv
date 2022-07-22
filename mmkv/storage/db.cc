#include "db.h"

#include "mmkv/protocol/status_code.h"
#include "mmkv/protocol/command.h"
#include "mmkv/util/memory_footprint.h"

namespace storage = mmkv::storage;
using namespace mmkv::storage;
using namespace mmkv::protocol;

MmkvDb storage::g_db;

#define DB g_db

#define SET_OK_VALUE(_value) \
  response->SetOk(); \
  response->SetValue(); \
  response->value = (_value)

#define SET_OK_VALUE_ \
  response->SetOk(); \
  response->SetValue()

#define SET_OK_VALUES(_values) \
  response->SetOk(); \
  response->SetValues(); \
  response->values = (_values)

#define SET_OK_VALUES_ \
  response->SetOk(); \
  response->SetValues()

#define SET_OK_VMEMBERS(_wms) \
  response->SetOk(); \
  response->SetVmembers(); \
  response->vmembers = (_wms)

#define SET_OK_VMEMBERS_ \
  response->SetOk(); \
  response->SetVmembers()

#define SET_OK_KVS(_kvs) \
  response->SetOk(); \
  response->SetKvs(); \
  response->kvs = (_kvs);

#define SET_OK_KVS_ \
  response->SetOk(); \
  response->SetKvs()

#define SET_OK_COUNT(_count) \
  response->SetOk(); \
  response->SetCount(); \
  response->count = (_count)

#define SET_OK_COUNT_ \
  response->SetOk(); \
  response->SetCount(); \

#define SET_XX_ELSE_CODE(xx) \
  if (code == S_OK) { \
    xx; \
  } else { \
    response->status_code = code; \
  }

void storage::DbExecute(MmbpRequest& request, MmbpResponse* response) {
  switch (request.command) {

    case STR_ADD: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "stradd");
      
      auto code = DB.InsertStr(std::move(request.key), std::move(request.value));
      if (response) {
        response->status_code = code;
      }
    }
      break;
    case STR_GET: {
      MMKV_ASSERT(request.HasKey(), "strget");

      String* str = nullptr;
      auto code = DB.GetStr(request.key, str);
      SET_XX_ELSE_CODE(SET_OK_VALUE(*str));
    }
      break;
    case STR_DEL: {
      MMKV_ASSERT(request.HasKey(), "strdel");

      auto code = DB.EraseStr(request.key);
      if (response) response->status_code = code;
    }
      break;

    case STR_SET: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "strset");
      auto code = DB.SetStr(std::move(request.key), std::move(request.value));
      if (response) response->status_code = code;
    }
      break;

    case STRLEN: {
      MMKV_ASSERT(request.HasKey(), "strlen");
      String* str = nullptr;
      auto code = DB.GetStr(request.key, str);
      SET_XX_ELSE_CODE(SET_OK_COUNT(str->size()));
    }
      break;

    case STRAPPEND: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "strappend");
      String* str = nullptr;
      auto code = DB.GetStr(request.key, str);
      if (code == S_OK) {
        str->append(request.value);
        if (response) response->status_code = code;
      }
    }
      break;

    case STRPOPBACK: {
      MMKV_ASSERT(request.HasKey() && request.HasCount(), "strpopback");
      String* str = nullptr;
      auto code = DB.GetStr(request.key, str);
      if (code == S_OK) {
        str->erase(str->size()-request.count, request.count);
        if (response) response->status_code = code;
      }
    }
      break;

    case LADD: {
      MMKV_ASSERT(request.HasKey() && request.HasValues(), "ladd");
      auto code = DB.ListAdd(std::move(request.key), request.values);
      if (response) response->status_code = code;
    }
      break;

    case LAPPEND:
    case LPREPEND: {
      MMKV_ASSERT(request.HasKey() && request.HasValues(), "lappend/lprepend");

      StatusCode code;
      switch (request.command) {
        case LAPPEND:
          code = DB.ListAppend(request.key, request.values);
          break;
        case LPREPEND:
          code = DB.ListPrepend(request.key, request.values);
          break;
      }

      if (response) response->status_code = code;
    }
      break;
    
    case LPOPFRONT:
    case LPOPBACK: {
      MMKV_ASSERT(request.HasKey() && request.HasCount(), "lpopback/popfront");

      StatusCode code;
      switch (request.command) {
        case LPOPFRONT:
          code = DB.ListPopFront(request.key, request.count);
          break;
        case LPOPBACK:
          code = DB.ListPopBack(request.key, request.count);
          break;
      }

      if (response) {
        SET_XX_ELSE_CODE(SET_OK_COUNT_)
      }
    }
      break;

    case LGETSIZE: {
      MMKV_ASSERT(request.HasKey(), "lgetsize");
      size_t count = 0;
      if ( (response->status_code = DB.ListGetSize(request.key, count)) == S_OK) {
        response->SetCount();
        response->count = count;
      }
    }
      break;

    case LGETRANGE: {
      MMKV_ASSERT(request.HasRange(), "lgetrange");

      if ( (response->status_code = DB.ListGetRange(request.key, response->values, request.range.left, request.range.right)) == S_OK) {
        response->SetValues();
      }
    }
      break;

    case LGETALL: {
      MMKV_ASSERT(request.HasKey(), "lgetrange/getall");
      
      if ( (response->status_code = DB.ListGetAll(request.key, response->values)) == S_OK) {
        response->SetValues();
      }
    }
      break;

    case LDEL: {
      MMKV_ASSERT(request.HasKey(), "ldel");
      auto code = DB.ListDel(request.key);
      if (response) response->status_code = code;
    }
      break;

    case VADD: {
      MMKV_ASSERT(request.HasKey() && request.HasVmembers(), "vadd");
      size_t count = 0;
      auto code = DB.VsetAdd(std::move(request.key), std::move(request.vmembers), count);
      if (response) {
        SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      }
    }
      break;

    case VALL: {
      MMKV_ASSERT(request.HasKey(), "vall");
      WeightValues values;
      if ( (response->status_code = DB.VsetAll(request.key, response->vmembers)) == S_OK) {
        response->SetVmembers();
      }
    }
      break;
    
    case VDELM: {
      MMKV_ASSERT(request.HasKey(), "vdelm");
      auto code = DB.VsetDel(request.key, request.value);
      if (response) response->status_code = code;
    }
      break;
    
    case VDELMRANGE: {
      MMKV_ASSERT(request.HasKey() && request.HasRange(), "vdelmrange");
      size_t count = 0;
      auto code = DB.VsetDelRange(request.key, request.range, count);
      if (response) { 
        SET_XX_ELSE_CODE(SET_OK_COUNT(count)) 
      }
    }
      break;

    case VDELMRANGEBYWEIGHT: {
      MMKV_ASSERT(request.HasKey() && request.HasRange(), "vdelmrangebyweight");
      size_t count = 0;
      auto code = DB.VsetDelRangeByWeight(request.key, request.GetWeightRange(), count);
      if (response) { 
        SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      }
    }
      break;
    
    case VSIZE: {
      MMKV_ASSERT(request.HasKey(), "vsize");
      size_t size;
      auto code = DB.VsetSize(request.key, size);
      SET_XX_ELSE_CODE(SET_OK_COUNT(size))
    }
      break;

    case VSIZEBYWEIGHT: {
      MMKV_ASSERT(request.HasKey() && request.HasRange(), "vsizebyweight");
      size_t size;
      auto code = DB.VsetSizeByWeight(request.key, request.GetWeightRange(), size);
      SET_XX_ELSE_CODE(SET_OK_COUNT(size))
    }
      break;
    
    case VWEIGHT: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "vweight");
      Weight weight;
      auto code = DB.VsetWeight(request.key, request.value, weight);
      SET_XX_ELSE_CODE(SET_OK_COUNT(util::double2u64(weight)))
    }
      break;

    case VORDER: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "vorder");
      size_t order = 0;
      auto code = DB.VsetOrder(request.key, request.value, order);
      SET_XX_ELSE_CODE(SET_OK_COUNT(order))
    }
      break;

    case VRORDER: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "vrorder");
      size_t order = 0;
      auto code = DB.VsetROrder(request.key, request.value, order);
      SET_XX_ELSE_CODE(SET_OK_COUNT(order))
    }
      break;

    case VRANGE: {
      MMKV_ASSERT(request.HasKey() && request.HasRange(), "vrange");
      auto code = DB.VsetRange(request.key, request.range, response->vmembers);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
    }
      break;

    case VRRANGE: {
      MMKV_ASSERT(request.HasKey() && request.HasRange(), "vrrange");
      auto code = DB.VsetRRange(request.key, request.range, response->vmembers);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
    }
      break;

    case VRANGEBYWEIGHT: {
      MMKV_ASSERT(request.HasKey() && request.HasRange(), "vrangebyweight");
      auto code = DB.VsetRangeByWeight(request.key, request.GetWeightRange(), response->vmembers);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)     
    }
      break;

    case VRRANGEBYWEIGHT: {
      MMKV_ASSERT(request.HasKey() && request.HasRange(), "vrrangebyweight");
      auto code = DB.VsetRRangeByWeight(request.key, request.GetWeightRange(), response->vmembers);

      SET_XX_ELSE_CODE(SET_OK_VMEMBERS_)
    }
      break;

    case MADD: {
      MMKV_ASSERT(request.HasKey() && request.HasKvs(), "madd");
      size_t count = 0;
      auto code = DB.MapAdd(std::move(request.key), std::move(request.kvs), count);

      if (response) { 
        SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      }
    }
      break;

    case MGET: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "mget");
      auto code = DB.MapGet(request.key, request.value, response->value);
      SET_XX_ELSE_CODE(SET_OK_VALUE_);
    }
      break;

    case MGETS: {
      MMKV_ASSERT(request.HasKey() && request.HasValues(), "mgets");
      auto code = DB.MapGets(request.key, request.values, response->values);
      SET_XX_ELSE_CODE(SET_OK_VALUES_);
    }

    case MSET: {
      MMKV_ASSERT(request.HasKey() && request.HasValues(), "mset");
      response->status_code = DB.MapSet(request.key, std::move(request.values[0]), std::move(request.values[1]));
    }
      break;

    case MDEL: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "mdel");
      auto code = DB.MapDel(request.key, request.value);
      if (response) response->status_code = code;
    }
      break;

    case MALL: {
      MMKV_ASSERT(request.HasKey(), "mall");
      auto code = DB.MapAll(request.key, response->kvs);

      SET_XX_ELSE_CODE(SET_OK_KVS_);
    }
      break;

    case MFIELDS: {
      MMKV_ASSERT(request.HasKey(), "mfields");
      auto code = DB.MapFields(request.key, response->values);
      SET_XX_ELSE_CODE(SET_OK_VALUES_);
    }
      break;

    case MVALUES: {
      MMKV_ASSERT(request.HasKey(), "mvalues");
      auto code = DB.MapValues(request.key, response->values);
      SET_XX_ELSE_CODE(SET_OK_VALUES_);
    }
      break;

    case MSIZE: {
      MMKV_ASSERT(request.HasKey(), "msize");
      size_t count = 0;
      auto code = DB.MapSize(request.key, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    }
      break;

    case MEXISTS: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "mexists");
      response->status_code = DB.MapExists(request.key, request.value);
    }
      break;
  
    case SADD: {
      MMKV_ASSERT(request.HasKey() && request.HasValues(), "sadd");
      size_t count = 0;
      auto code = DB.SetAdd(std::move(request.key), request.values, count);
      if (response) {
        SET_XX_ELSE_CODE(SET_OK_COUNT(count))
      }
    }
      break;
    
    case SDELM: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "sdelm");
      auto code = DB.SetDelm(request.key, request.value);
      if (response) response->status_code = code;
    }
      break;

    case SEXISTS: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "sexists");
      response->status_code = DB.SetExists(request.key, request.value);
    }
      break;
    
    case SSIZE: {
      MMKV_ASSERT(request.HasKey(), "ssize");
      size_t count = 0;
      response->status_code = DB.SetSize(request.key, count);
      if (response->status_code == S_OK) {
        response->count = count;
        response->SetCount();
      }
    }
      break;
    
    case SALL: {
      MMKV_ASSERT(request.HasKey(), "sall");
      response->status_code = DB.SetAll(request.key, response->values);
      if (response->status_code == S_OK)
        response->SetValues();
    }
      break;
    
    case SAND: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "sand");
      response->status_code = DB.SetAnd(request.key, request.value, response->values);
      if (response->status_code == S_OK)
        response->SetValues();
    }
      break;

    case SOR: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "sor");
      response->status_code = DB.SetOr(request.key, request.value, response->values);
      if (response->status_code == S_OK)
        response->SetValues();
    }
      break;

    case SSUB: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "ssub");
      response->status_code = DB.SetSub(request.key, request.value, response->values);
      if (response->status_code == S_OK)
        response->SetValues();
    }
      break;

    case SANDTO: {
      MMKV_ASSERT(request.HasValues(), "sandto");
      response->status_code = DB.SetAndTo(request.values[1], request.values[2], std::move(request.values[0]));
    }
      break;

    case SORTO: {
      MMKV_ASSERT(request.HasValues(), "sorto");
      response->status_code = DB.SetOrTo(request.values[1], request.values[2], std::move(request.values[0]));
    }
      break;

    case SSUBTO: {
      MMKV_ASSERT(request.HasValues(), "ssubto");
      response->status_code = DB.SetSubTo(request.values[1], request.values[2], std::move(request.values[0]));
    }
      break;

    case SANDSIZE: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "sandsize");
      size_t count = 0;
      auto code = DB.SetAndSize(request.key, request.value, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    }
      break;

    case SORSIZE: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "sorsize");
      size_t count = 0;
      auto code = DB.SetOrSize(request.key, request.value, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    }
      break;

    case SSUBSIZE: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "ssubsize");
      size_t count = 0;
      auto code = DB.SetSubSize(request.key, request.value, count);
      SET_XX_ELSE_CODE(SET_OK_COUNT(count));
    }
      break;
    case MEM_STAT: {
      MMKV_ASSERT(request.HasNone(), "memorystat");
      SET_OK_VALUE(util::GetMemoryStat());
    }
      break;

    case TYPE: {
      MMKV_ASSERT(request.HasKey(), "type");
      db::DataType type;
      if (DB.Type(request.key, type)) {
        SET_OK_VALUE(GetDataTypeString(type));
      } else {
        response->status_code = S_NONEXISTS;
      }
    }
      break;

    case DEL: {
      MMKV_ASSERT(request.HasKey(), "del");
      auto success = DB.Delete(request.key);
      if (response) response->status_code = success ? S_OK : S_NONEXISTS;
    }
      break;

    case RENAME: {
      MMKV_ASSERT(request.HasKey() && request.HasValue(), "rename");
      auto code = DB.Rename(request.key, std::move(request.value));
      if (response) response->status_code = code;
    }
      break;

    case KEYALL: {
      DB.GetAllKeys(response->values);
      response->SetValues();
      response->SetOk();
    }
      break;
  }

}