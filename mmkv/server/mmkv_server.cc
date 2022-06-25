#include "mmkv_server.h"

using namespace kanon;
using namespace mmkv::server;

MmkvServer::MmkvServer(EventLoop* loop, InetAddr const& addr)
  : server_(loop, addr, "In-Memory Key-Value database")
{

}
