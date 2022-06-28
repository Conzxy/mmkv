#include "translator.h"

#include <iostream>

#include "mmkv/client/information.h"

#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/util/tokenizer.h"

using namespace mmkv::protocol;
using namespace mmkv::util;
using namespace kanon;
using namespace std;

#define SYNTAX_ERROR_ROUTINE \
  if (++token_iter == tokenizer.end()) { \
    return E_SYNTAX_ERROR; \
  }

#define SYNTAX_ERROR_ROUTINE_END \
  if (++token_iter != tokenizer.end()) { \
    return E_SYNTAX_ERROR; \
  }

#define TO_MMKV_STRING(token) \
  mmkv::algo::String((token).data(), (token).size())

Translator::ErrorCode Translator::Parse(MmbpRequest* request, StringView statement) {
  Tokenizer tokenizer(statement);
  
  auto token_iter = tokenizer.begin();
  
  if (token_iter == tokenizer.end()) {
    return E_NO_COMMAND;
  }
  
  auto cmd = *token_iter; 
  request->SetCommand(command_map[cmd]);

#define SET_KEY \
  SYNTAX_ERROR_ROUTINE \
  request->SetKey(TO_MMKV_STRING(*token_iter));

#define SET_VALUE \
  SYNTAX_ERROR_ROUTINE \
  request->SetValue(TO_MMKV_STRING(*token_iter));

  if (cmd == command_strings[STR_ADD] ||
      cmd == command_strings[STR_SET]) {
    SET_KEY
    SET_VALUE
    SYNTAX_ERROR_ROUTINE_END
  } else if (cmd == command_strings[STR_GET] ||
             cmd == command_strings[STR_DEL]) {
    SET_KEY
    SYNTAX_ERROR_ROUTINE_END
  } else if (cmd == command_strings[MEM_STAT]) {
    SYNTAX_ERROR_ROUTINE_END
  } else if (cmd == "help") {
    std::cout << HELP_INFORMATION;
    return E_INVALID_COMMAND;
  } else if (cmd == "exit") {
    return E_EXIT;
  } else {
    std::cout << "ERROR: invalid command: " << cmd.ToString() << "\n";
    std::cout << HELP_INFORMATION;
    return E_INVALID_COMMAND;
  }

  return E_OK; 
}
