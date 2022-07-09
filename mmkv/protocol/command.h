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
  MEM_STAT,
  EXPIRE_AT,
  EXPIRE_AFTER,
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
  
  // common
  DEL,
  RENAME,
  TYPE,
  COMMAND_NUM,
};

extern std::string command_strings[];

} // protocol
} // mmkv

#endif // _MMKV_PROTOCOL_COMMAND_H_
