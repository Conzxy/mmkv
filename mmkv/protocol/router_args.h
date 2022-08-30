#ifndef MMKV_PROTOCOL_ROUTER_ARGS_H__
#define MMKV_PROTOCOL_ROUTER_ARGS_H__

#include "mmbp.h"

namespace mmkv {
namespace protocol {

class RouterArgs :public MmbpMessage {
 public:
  void ParseFrom(Buffer &buffer) override;
  void SerializeTo(ChunkList &buffer) const override;
  RouterArgs *New() const override { return new RouterArgs(); }
  
  static RouterArgs *prototype;
  std::string key;
};

} // protocol
} // mmkv
 
#endif // MMKV_PROTOCOL_ROUTER_ARGS_H__
