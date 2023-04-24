// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_PROTOCOL_MMBP_TYPE_H_
#define _MMKV_PROTOCOL_MMBP_TYPE_H_

#include <stdint.h>

namespace mmkv {
namespace protocol {

using CommandField = uint16_t;
using ExpireTimeField= uint64_t;
using CountField = uint32_t;

} // protocol
} // mmkv

#endif