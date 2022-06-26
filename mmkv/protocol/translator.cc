#include "translator.h"
#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_request.h"

using namespace mmkv::protocol;
using namespace kanon;
using namespace std;

Translator::ErrorCode Translator::Parse(MmbpRequest* request, StringView statement) {
  #define STATEMENT_SETCOMMAND(cmd) \
    request->SetCommand(cmd); \
    statement.remove_prefix(command_strings[cmd].size());

  if (statement.starts_with(command_strings[STR_ADD])) {
    STATEMENT_SETCOMMAND(STR_ADD)
    
  } else if (statement.starts_with(command_strings[STR_GET])) {
    STATEMENT_SETCOMMAND(STR_GET)
  } else if (statement.starts_with(command_strings[STR_DEL])) {
    STATEMENT_SETCOMMAND(STR_DEL);
  } else if (statement.starts_with(command_strings[STR_SET])) {
    STATEMENT_SETCOMMAND(STR_SET);
  } else if (statement.starts_with(command_strings[MEM_STAT])) {
    STATEMENT_SETCOMMAND(MEM_STAT);
  } else {
    return E_INVALID_COMMAND;
  }

  return E_OK; 
}
