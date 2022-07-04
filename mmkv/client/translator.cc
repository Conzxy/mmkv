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
  auto integer = TO_MMKV_STRING(*token_iter); \
  char* end; \
  var = ::strtol(integer.c_str(), &end, 10); \
  if (integer.c_str() == end) { \
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
  request->command = command_map[cmd];

  if (cmd == command_strings[STR_ADD] ||
      cmd == command_strings[STR_SET]) {
    SET_KEY;
    SET_VALUE;
    SYNTAX_ERROR_ROUTINE_END;
  } else if (cmd == command_strings[STR_GET] ||
             cmd == command_strings[STR_DEL] ||
             cmd == command_strings[LGETSIZE] ||
             cmd == command_strings[LGETALL] || 
             cmd == command_strings[LDEL]) {
    SET_KEY;
    SYNTAX_ERROR_ROUTINE_END;
  } else if (cmd == command_strings[MEM_STAT]) {
    SYNTAX_ERROR_ROUTINE_END;
  } else if (cmd == "help") {
    std::cout << HELP_INFORMATION;
    return E_INVALID_COMMAND;
  } else if (cmd == "exit" || cmd == "quit") {
    return E_EXIT;
  } else if (cmd == command_strings[LADD] ||
             cmd == command_strings[LAPPEND] ||
             cmd == command_strings[LPREPEND]) {
    SET_KEY;
    SET_VALUES;
  } else if (cmd == command_strings[LGETRANGE]) {
    SET_KEY;
    uint32_t range[2];
    SET_INTEGER(range[0], "ERROR: left bound of range is invalid");
    SET_INTEGER(range[1], "ERROR: right bound of range is invalid"); 
    request->SetRange();
    request->range = MmbpRequest::Range{range[0], range[1]};

  } else if (cmd == command_strings[LPOPFRONT] ||
             cmd == command_strings[LPOPBACK]) {
    SET_KEY;
    uint32_t count;
    SET_INTEGER(count, "ERROR: count is invalid");
    request->SetCount();
    request->count = count;
  } else {
    std::cout << "ERROR: invalid command: " << cmd.ToString() << "\n";
    std::cout << HELP_INFORMATION;
    return E_INVALID_COMMAND;
  }

  return E_OK; 
}
