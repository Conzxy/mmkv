#include "mmkv/server/mmkv_server.h"

using namespace kanon;
using namespace mmkv;
using namespace mmkv::server;

int main() {
  EventLoop loop;
  MmkvServer server(&loop);
  
  server.Start();
  loop.StartLoop();
}
