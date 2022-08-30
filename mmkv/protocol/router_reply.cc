#include "router_reply.h"
#include "mmbp_util.h"

using namespace mmkv::protocol;

static RouterReply reply;
RouterReply *RouterReply::prototype = &reply;

void RouterReply::ParseFrom(Buffer &buffer) {
  SetField(address, buffer, true);
  SetField(port, buffer);
}

void RouterReply::SerializeTo(ChunkList &buffer) const {
  SerializeField(address, buffer, true);
  SerializeField(port, buffer);
}
