#include "mmkv/server/tracker.h"
#include "mmkv/server/sharder.h"

using namespace mmkv::server;

int main() {
  kanon::SetKanonLog(false);

  EventLoop loop;
  Tracker tracker(&loop);
  tracker.Listen();

  loop.StartLoop();
}