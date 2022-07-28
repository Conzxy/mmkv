#ifndef _MMKV_SERVER_MMKV_SERVER_H_
#define _MMKV_SERVER_MMKV_SERVER_H_

#include "kanon/util/noncopyable.h"
#include "kanon/net/user_server.h"

#include "mmkv/db/kvdb.h"

namespace mmkv {
namespace server {

class MmkvSession;

class MmkvServer {
  DISABLE_EVIL_COPYABLE(MmkvServer)
  friend class MmkvSession;
  
 public:
  explicit MmkvServer(EventLoop* loop, InetAddr const& addr=InetAddr("127.0.0.1:9998"));
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
};

} // server
} // mmkv

#endif // _MMKV_SERVER_MMKV_SERVER_H_
