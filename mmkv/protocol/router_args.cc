#include "router_args.h"

#include "mmbp_util.h"

using namespace mmkv::protocol;

static RouterArgs args;
RouterArgs *RouterArgs::prototype = &args;

void RouterArgs::ParseFrom(Buffer &buffer)
{
  ParseComponent(key, buffer, true);
}

void RouterArgs::SerializeTo(ChunkList &buffer) const
{
  SerializeComponent(key, buffer, true);
}
