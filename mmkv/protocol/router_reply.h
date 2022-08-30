#ifndef MMKV_PROTOCOL_ROUTER_REPLY_H__
#define MMKV_PROTOCOL_ROUTER_REPLY_H__

#include "mmbp.h"

namespace mmkv{
namespace protocol {

class RouterReply :public MmbpMessage {
 public:
  void ParseFrom(Buffer &buffer) override;
  void SerializeTo(ChunkList &buffer) const override;
  RouterReply *New() const override { return new RouterReply(); }
  
  static RouterReply *prototype;

  std::string address;
  uint16_t port;
};

} // protocol
} // mmkv

#endif // MMKV_PROTOCOL_ROUTER_REPLY_H__
