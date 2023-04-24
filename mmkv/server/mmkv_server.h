// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_SERVER_MMKV_SERVER_H_
#define _MMKV_SERVER_MMKV_SERVER_H_

#include "kanon/util/noncopyable.h"
#include "kanon/net/user_server.h"

#include "mmkv/db/kvdb.h"

#include "mmkv/tracker/tracker_client.h"

namespace mmkv {
namespace server {

class MmkvSession;

class MmkvServer {
  DISABLE_EVIL_COPYABLE(MmkvServer)
  friend class MmkvSession;
  
 public:
  explicit MmkvServer(EventLoop* loop, InetAddr const& addr, InetAddr const &sharder_addr);
  ~MmkvServer() noexcept;

  void Listen() {
    server_.StartRun();
  }

  void SetLoopNum(int num) {
    server_.SetLoopNum(num);
  }

  void Start();
 private:
  TcpServer server_;

  std::unique_ptr<EventLoopThread> tracker_cli_loop_thr_;
  std::unique_ptr<TrackerClient> tracker_cli_;
};

} // server
} // mmkv

#endif // _MMKV_SERVER_MMKV_SERVER_H_
