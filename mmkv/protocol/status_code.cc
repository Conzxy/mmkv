// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "status_code.h"

#include <cstdio>
#include <cstdlib>

using namespace mmkv::protocol;

char const *mmkv::protocol::GetStatusMessage(mmkv::protocol::StatusCode code) noexcept
{
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
    case S_EXISTS_DIFF_TYPE:
      return "ERROR: Key exists but with different type";
    case S_FIELD_NONEXISTS:
      return "ERROR: Field doesn't exists";
    case S_SET_MEMBER_NONEXISTS:
      return "ERROR: Member doesn't exists in set";
    case S_INVALID_REQUEST:
      return "ERROR: Invalid request(client error)";
    case S_SET_NO_MEMBER:
      return "ERROR: No member in the set";
    case S_DEST_EXISTS:
      return "ERROR: The destination set already exists";
    case S_EXPIRE_DISABLE:
      return "ERROR: The expiration is disable";
    case S_SHARD_LOCKED:
      return "ERROR: The shard which key belonging is locked";
    default:
      fprintf(stderr, "There are some status code message aren't added");
      abort();
  }

  return "ERROR: Unknown error";
}

char const *mmkv::protocol::StatusCode2Str(StatusCode code) noexcept
{
  switch (code) {
    case S_OK:
      return "ok";
    case S_EXISTS:
      return "exists";
    case S_NONEXISTS:
      return "nonexists";
    case S_INVALID_MESSAGE:
      return "invalid message";
    case S_INVALID_RANGE:
      return "invalid range";
    case S_VMEMBER_NONEXISTS:
      return "vmember nonexists";
    case S_EXISTS_DIFF_TYPE:
      return "exists diff type";
    case S_FIELD_NONEXISTS:
      return "field nonexists";
    case S_SET_MEMBER_NONEXISTS:
      return "set member nonexists";
    case S_INVALID_REQUEST:
      return "invalid request";
    case S_SET_NO_MEMBER:
      return "set no member";
    case S_DEST_EXISTS:
      return "dest exists";
    case S_EXPIRE_DISABLE:
      return "expire disable";
    case S_SHARD_PROCESSING:
      return "Shard is processing";
    default:
      return "Unknown status code";
  }
}
