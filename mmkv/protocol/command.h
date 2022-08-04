#ifndef _MMKV_PROTOCOL_COMMAND_H_
#define _MMKV_PROTOCOL_COMMAND_H_

#include <stdint.h>
#include <string>

namespace mmkv {
namespace protocol {

enum Command : uint16_t {
  // string operation
  STR_ADD = 0,
  STR_DEL,
  STR_GET,
  STR_SET,
  STRLEN,
  STRAPPEND,
  STRPOPBACK,
  // list operation
  LADD,
  LAPPEND,
  LPREPEND,
  LGETSIZE,
  LGETALL,
  LGETRANGE,
  LPOPFRONT,
  LPOPBACK,
  LDEL,
  // sorted set operation
  VADD,
  VDELM,
  VDELMRANGE,
  VDELMRANGEBYWEIGHT,
  VSIZE,
  VSIZEBYWEIGHT,
  VWEIGHT,
  VORDER,
  VRORDER,
  VRANGE,
  VALL,
  VRANGEBYWEIGHT,
  VRRANGE,
  VRRANGEBYWEIGHT,
  // map operation
  MADD,
  MSET,
  MGET,
  MGETS,
  MDEL,
  MALL,
  MFIELDS,
  MVALUES,
  MSIZE,
  MEXISTS,
  // hash set operation
  SADD,
  SDELM,
  SSIZE,
  SALL,
  SEXISTS,
  SAND,
  SANDTO,
  SSUB,
  SSUBTO,
  SOR,
  SORTO,
  SANDSIZE,
  SSUBSIZE,
  SORSIZE,
  SRANDDELM,
  // common
  MEM_STAT,
  EXPIRE_AT,
  EXPIRE_AFTER,
  EXPIREM_AT,
  EXPIREM_AFTER,
  PERSIST,
  EXPIRATION,
  TTL,
  DEL,
  RENAME,
  TYPE,
  KEYALL,
  COMMAND_NUM,
};

namespace detail {
extern std::string command_strings[];
} // detail

inline std::string const &GetCommandString(Command cmd) {
  return detail::command_strings[cmd];
}

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_COMMAND_H_
