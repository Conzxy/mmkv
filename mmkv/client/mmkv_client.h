#ifndef _MMKV_CLIENT_MMKV_CLIENT_H_
#define _MMKV_CLIENT_MMKV_CLIENT_H_

#include "mmkv/protocol/mmbp_codec.h"
#include "mmkv/protocol/mmbp_response.h"

#include <kanon/net/user_client.h>
#include <kanon/thread/condition.h>
#include <kanon/thread/mutex_lock.h>
#include <kanon/util/noncopyable.h>

namespace mmkv {
namespace client {

class MmkvClient {
  DISABLE_EVIL_COPYABLE(MmkvClient)
  
 public:
  MmkvClient(EventLoop* loop, InetAddr const& server_addr);
  ~MmkvClient() noexcept;

  void Start();
  void ConsoleIoProcess();
  void IoWait() {
    io_cond_.Wait();
  }

 private:
  TcpClient client_;
  protocol::MmbpCodec codec_;

  kanon::Condition io_cond_;
  kanon::MutexLock mutex_;
};

} // client
} // mmkv

#endif // _MMKV_CLIENT_MMKV_CLIENT_H_
