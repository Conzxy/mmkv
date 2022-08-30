#include "mmkv/server/tracker.h"
#include "mmkv/server/tracker_client.h"

using namespace mmkv::server;
using namespace kanon;

int main() {
  EventLoop loop;
  Tracker tracker(&loop);
  tracker.Listen();

  EventLoopThread loop_thr;
  auto loop2 = loop_thr.StartRun();
  InetAddr tracker_addr("127.0.0.1:19997");

  std::vector<std::unique_ptr<TrackerClient>> clients;
  for (int i = 0; i < 3; ++i) {
    clients.emplace_back(kanon::make_unique<TrackerClient>(loop2, tracker_addr));
    clients[i]->Connect();
  }

  loop.StartLoop();
}