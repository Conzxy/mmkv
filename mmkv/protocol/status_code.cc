#include "status_code.h"
#include <sys/socket.h>

using namespace mmkv::protocol;

char const* mmkv::protocol::GetStatusMessage(mmkv::protocol::StatusCode code) noexcept {
  switch (code) {
  case S_OK:
    return "OK";
  case S_EXISTS:
    return "ERROR: Key already exists";
  case S_NONEXISTS:
    return "ERROR: Key doesn't exists";
  case S_INVALID_MESSAGE:
    return "ERROR: Invalid message";
  case S_INVALID_RANGE:
    return "ERROR: Invalid range";
  case S_VMEMBER_NONEXISTS:
    return "ERROR: Member doesn't exists in vset";
  case S_EXISITS_DIFF_TYPE:
    return "ERROR: Key exists but with different type" ;
  case S_FIELD_NONEXISTS:
    return "ERROR: Field doesn't exists";
  case S_SET_MEMBER_NONEXISTS:
    return "ERROR: Member doesn't exists in set";
  }
  
  return "ERROR: Unknown error";
}

char const* mmkv::protocol::StatusCode2Str(StatusCode code) noexcept {
  switch (code) {
    case S_OK:
      return "Ok";
    case S_EXISTS:
      return "EXISTS";
    case S_NONEXISTS:
      return "Nonexists";
    case S_INVALID_MESSAGE:
      return "Invalid message";
    case S_INVALID_RANGE:
      return "Invalid range";
    case S_VMEMBER_NONEXISTS:
      return "Vmember nonexists";
    case S_EXISITS_DIFF_TYPE:
      return "exists with diff type";
    case S_FIELD_NONEXISTS:
      return "field nonexists";
    case S_SET_MEMBER_NONEXISTS:
      return "set member nonexists";
    default:
      return "Unknown status code";
  }
}
