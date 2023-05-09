// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_SHARD_CONTROLLER_CODEC_H__
#define _MMKV_SHARD_CONTROLLER_CODEC_H__

#include <kanon/protobuf/protobuf_codec2.h>

namespace mmkv {
namespace server {

DEF_SPECIFIC_TAG_PROTOBUF_CODEC(ShardControllerCodec, "SCPB", (1 << 26));

} // namespace server
} // namespace mmkv
#endif
