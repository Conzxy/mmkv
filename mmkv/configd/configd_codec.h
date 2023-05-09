#ifndef _MMKV_CONFIGD_CODEC_H__
#define _MMKV_CONFIGD_CODEC_H__

#include <kanon/protobuf/protobuf_codec2.h>

namespace mmkv {

DEF_SPECIFIC_TAG_PROTOBUF_CODEC(ConfigdCodec, "CDPB", (1 << 26));

} // namespace mmkv

#endif
