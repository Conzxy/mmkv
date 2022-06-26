#include "mmkv/client/mmkv_client.h"

#include <kanon/net/user_client.h>

using namespace mmkv::client;
using namespace kanon;

int main(int argc, char* argv[]) {
  uint16_t port = 9998;
  std::string ip = "127.0.0.1";

  if (argc > 1) {
    port = ::atoi(argv[1]);
  }

  if (argc > 2) {
    ip = argv[2];
  }

  EventLoopThread loop_thread;
  
  auto loop = loop_thread.StartRun(); 

  InetAddr server_addr(ip, port);
  MmkvClient client(loop, server_addr);
  
  client.Start();
  
  while (1) {
    client.IoWait();
    client.ConsoleIoProcess();
  }
}
