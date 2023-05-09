// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_FORWARDER_CODEC_H__
#define _MMKV_FORWARDER_CODEC_H__

#include <kanon/protobuf/protobuf_codec2.h>

namespace mmkv {
namespace server {

#define FORWARDER_TAG      "FTPB"
#define FORWARDER_MAX_SIZE (1 << 16)

class ForwarderCodec : public kanon::protobuf::ProtobufCodec2 {
  using Base = kanon::protobuf::ProtobufCodec2;

 public:
  ForwarderCodec()
    : Base(FORWARDER_TAG, FORWARDER_MAX_SIZE)
  {
  }
};

} // namespace server
} // namespace mmkv
#endif
