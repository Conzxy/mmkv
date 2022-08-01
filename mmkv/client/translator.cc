#include "translator.h"

#include <cstdint>

#include "mmkv/client/information.h"

#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/util/tokenizer.h"
#include "mmkv/util/print_util.h"

using namespace mmkv::protocol;
using namespace mmkv::util;
using namespace kanon;
using namespace std;

#define SYNTAX_ERROR_ROUTINE do { \
  if (++token_iter == tokenizer.end()) { \
    return E_SYNTAX_ERROR; \
  } } while (0)

#define SYNTAX_ERROR_ROUTINE_END do { \
  if (++token_iter != tokenizer.end()) { \
    return E_SYNTAX_ERROR; \
  } } while (0)

#define TO_MMKV_STRING(token) \
  mmkv::algo::String((token).data(), (token).size())

#define SET_KEY do { \
  SYNTAX_ERROR_ROUTINE; \
  request->SetKey(); \
  request->key = TO_MMKV_STRING(*token_iter); \
  } while(0)

#define SET_VALUE do { \
  SYNTAX_ERROR_ROUTINE; \
  request->SetValue(); \
  request->value = TO_MMKV_STRING(*token_iter); \
  } while (0)

#define SET_VALUES do { \
  SYNTAX_ERROR_ROUTINE; \
  StrValues values; \
  for (;;) { \
    values.push_back(TO_MMKV_STRING(*token_iter)); \
    ++token_iter; \
    if (token_iter == tokenizer.end()) break; \
  } \
  request->SetValues(); \
  request->values = std::move(values); \
  } while (0)

#define SET_INTEGER(var, msg) do { \
  SYNTAX_ERROR_ROUTINE; \
  char buf[64]; \
  auto token = *token_iter; \
  ::memcpy(buf, token.data(), token.size()); \
  buf[token.size()] = 0; \
  char* end; \
  (var) = ::strtoull(buf, &end, 10); \
  if ((var) == 0 && buf == end) { \
    ErrorPrintf(msg); \
    return E_SYNTAX_ERROR; \
  } } while (0)

#define SET_DOUBLE(var, msg) do { \
  char buf[128]; \
  auto token = *token_iter; \
  ::memcpy(buf, (token).data(), (token).size()); \
  buf[(token).size()] = 0; \
  char* end; \
  (var) = ::strtod(buf, &end); \
  if ((var) == 0 && end == buf) { \
    ErrorPrintf(msg); \
    return E_SYNTAX_ERROR; \
  } } while (0)

Translator::ErrorCode Translator::Parse(MmbpRequest* request, StringView statement) {
  if (statement.size() > 0 && statement[0] == '!') {
    if (::system(statement.substr(1).data()) == 0)
      return E_SHELL_CMD;
    else
      return E_INVALID_COMMAND;
  }

  Tokenizer tokenizer(statement);
  
  auto token_iter = tokenizer.begin();
  
  if (token_iter == tokenizer.end()) {
    return E_NO_COMMAND;
  }

  auto cmd = *token_iter;
  
  /* The type of `cmd` is StringView, so data() return char const*.
   * Force convert it to char* and transform the upper case character
   * to lower case.
   * That's unsafe but also OK in this case.
   */
  char *cmd_data = (char*)cmd.data();
  for (size_t i = 0; i < cmd.size(); ++i) {
    if (cmd[i] >= 'A' && cmd[i] <= 'Z') {
      /* A-Z: starts with 0x41
       * a-z: starts with 0x61
       */
      cmd_data[i] += 0x20;
    }
  }

  const auto valid_cmd = GetCommand(cmd, request->command);

  if (!valid_cmd) {
    ErrorPrintf("ERROR: invalid command: %s\n", cmd.ToString().c_str());
    // std::cout << GetHelp();
    return E_INVALID_COMMAND;
  }

  switch (GetCommandFormat(cmd)) {
    case F_NONE:
      break;
    case F_VALUE: {
      SET_KEY;
      SET_VALUE;
      SYNTAX_ERROR_ROUTINE_END;
    }
      break;
    case F_ONLY_KEY: {
      SET_KEY;
      SYNTAX_ERROR_ROUTINE_END;
    }
      break;
    case F_VALUES: {
      SET_KEY;
      SET_VALUES;
    }
      break;

    case F_SET_OP: {
      SET_KEY;
      SET_VALUE;
      SYNTAX_ERROR_ROUTINE_END;
    }
      break;
    
    case F_SET_OP_TO: {
      SET_VALUES;
      if (request->values.size() > 3) {
        return E_SYNTAX_ERROR;
      }
    }
      break;

    case F_FIELD_VALUE: {
      SET_KEY;

      auto& values = request->values;
      values.reserve(2);
      SYNTAX_ERROR_ROUTINE;
      values.push_back(TO_MMKV_STRING(*token_iter));
      SYNTAX_ERROR_ROUTINE;
      values.push_back(TO_MMKV_STRING(*token_iter));
      SYNTAX_ERROR_ROUTINE_END;
      request->SetValues();
    }
      break;

    case F_RANGE: {
      SET_KEY;
      uint32_t range[2];
      SET_INTEGER(range[0], "ERROR: left bound of range is invalid");
      SET_INTEGER(range[1], "ERROR: right bound of range is invalid"); 
      request->SetRange();
      request->range = Range{range[0], range[1]};
      SYNTAX_ERROR_ROUTINE_END;
    }
      break;
    case F_COUNT: {
      SET_KEY;
      // FIXME uint64_t
      uint32_t count;
      SET_INTEGER(count, "ERROR: count is invalid");
      request->SetCount();
      request->count = count;
      SYNTAX_ERROR_ROUTINE_END;
    }
      break;
    case F_VSET_MEMBERS: {
      SET_KEY;
      auto& vms = request->vmembers;

      while (++token_iter != tokenizer.end()) {
        double weight;
        SET_DOUBLE(weight, "ERROR: invalid weight");
        SYNTAX_ERROR_ROUTINE;
        auto member = TO_MMKV_STRING(*token_iter);
        vms.push_back({weight, std::move(member)});
      }
      request->SetVmembers();
    }
      break;
    case F_DRANGE: {
      SET_KEY;

      double drange[2];
      SYNTAX_ERROR_ROUTINE;
      SET_DOUBLE(drange[0], "ERROR: left weight of range is invalid");
      SYNTAX_ERROR_ROUTINE;
      SET_DOUBLE(drange[1], "ERROR: right weight of range is invalid");
      SYNTAX_ERROR_ROUTINE_END;
      request->SetWeightRange(drange[0], drange[1]);
    }
      break;
    case F_HELP: {
      ::fputs(GetHelp().c_str(), stdout);
      return E_INVALID_COMMAND;
    }
      break;
    case F_EXIT:
      return E_EXIT;
    case F_MAP_VALUES: {
      SET_KEY;
      auto& kvs = request->kvs;
      while (++token_iter != tokenizer.end()) {
        StrKeyValue kv;
        kv.key = TO_MMKV_STRING(*token_iter);
        SYNTAX_ERROR_ROUTINE;
        kv.value = TO_MMKV_STRING(*token_iter);
        kvs.push_back(std::move(kv));
      }
      request->SetKvs();
    }
      break;
    case F_EXPIRE: {
      SET_KEY;
      SET_INTEGER(request->expire_time, "ERROR: expiration time is invalid");
      request->SetExpireTime();
      SYNTAX_ERROR_ROUTINE_END;
    } 
      break;
    default:
      assert(false && "This must be a valid command");
  } 

  request->DebugPrint(); 
  return E_OK; 
}
