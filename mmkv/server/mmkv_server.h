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
  friend class Session;
  
 public:
  explicit MmkvServer(EventLoop* loop, InetAddr const& addr=InetAddr("127.0.0.1:9998"));
    
 private:
  TcpServer server_;

  db::MmkvDb db_;  
};

} // server
} // mmkv

#endif // _MMKV_SERVER_MMKV_SERVER_H_
