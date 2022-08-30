#include "mmkv/server/sharder.h"
#include "mmkv/server/tracker_client.h"
#include "mmkv/server/config.h"

using namespace mmkv::server;

int main(int argc, char *argv[]) {
  // kanon::SetKanonLog(false);

  EventLoop loop;
  std::string basename = argv[2];
  std::string cli_name = basename + " Client" ;
  auto cli = new TrackerClient(&loop, InetAddr("127.0.0.1:19997"), atoi(argv[1]), cli_name, basename);
  cli->Connect();

  loop.StartLoop();
}