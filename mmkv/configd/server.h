// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_CONFIGD_SERVER_H_
#define MMKV_CONFIGD_SERVER_H_

#include <kanon/util/noncopyable.h>
#include <kanon/net/user_server.h>

#include "mmkv/tracker/tracker.h"

namespace mmkv {
namespace configd {

/**
 */
class ConfigServer : kanon::noncopyable {
 public:
  ConfigServer(EventLoop *loop, InetAddr const &addr, InetAddr const &tracker_addr);
  ~ConfigServer() noexcept;

  void Listen() { router_.StartRun(); }
 private:
  TcpServer router_;

  EventLoopThread tracker_loop_thr_;
  server::Tracker tracker_;
};

} // config
} // mmkv

#endif // MMKV_CONFIGD_SERVER_H_
