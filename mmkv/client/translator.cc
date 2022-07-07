#include "translator.h"

#include <cstdint>
#include <iostream>

#include "mmkv/client/information.h"

#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/util/tokenizer.h"

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
  (var) = ::strtol(buf, &end, 10); \
  if ((var) == 0 && buf == end) { \
    std::cout << (msg) << "\n"; \
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
    std::cout << (msg) << "\n"; \
    return E_SYNTAX_ERROR; \
  } } while (0)

Translator::ErrorCode Translator::Parse(MmbpRequest* request, StringView statement) {
  Tokenizer tokenizer(statement);
  
  auto token_iter = tokenizer.begin();
  
  if (token_iter == tokenizer.end()) {
    return E_NO_COMMAND;
  }
  
  auto cmd = *token_iter;
  auto success = command_map.find(cmd);
  auto format_iter = command_formats.find(cmd);

  if (success != command_map.end()) {
    request->command = success->second;
  } else {
    if (format_iter == command_formats.end()) {
      std::cout << "ERROR: invalid command: " << cmd.ToString() << "\n";
      std::cout << HELP_INFORMATION;
      return E_INVALID_COMMAND;
    }
  }

  switch (format_iter->second) {
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
      std::cout << HELP_INFORMATION;
      return E_INVALID_COMMAND;
    }
      break;
    case F_EXIT:
      return E_EXIT;
    default:
      assert(false && "This must be a valid command");
  } 

  request->DebugPrint(); 
  return E_OK; 
}
