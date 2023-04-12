#include "router_reply.h"
#include "mmbp_util.h"

using namespace mmkv::protocol;

static RouterReply reply;
RouterReply *RouterReply::prototype = &reply;

void RouterReply::ParseFrom(Buffer &buffer)
{
  ParseComponent(address, buffer, true);
  ParseComponent(port, buffer);
}

void RouterReply::SerializeTo(ChunkList &buffer) const
{
  SerializeComponent(address, buffer, true);
  SerializeComponent(port, buffer);
}
