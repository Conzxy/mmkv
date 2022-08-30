#ifndef MMKV_SERVER_ROUTER_H_
#define MMKV_SERVER_ROUTER_H_

#include <kanon/util/noncopyable.h>
#include <kanon/net/user_server.h>

#include "mmkv/tracker/tracker.h"

namespace mmkv {
namespace server {

/**
 * \brief Route the request to appropriate server instance
 *
 * Use the Hash(key) to shard index.
 * The router is running in the other thread loop.
 */
class Router : kanon::noncopyable {
 public:
  Router(EventLoop *loop, InetAddr const &addr, InetAddr const &tracker_addr);
  ~Router() noexcept;

  void Listen() { router_.StartRun(); }
 private:
  TcpServer router_;

  EventLoopThread tracker_loop_thr_;
  Tracker tracker_;
};

} // mmkv
} // server

#endif // MMKV_SERVER_ROUTER_H_
