#ifndef _MMKV_CLIENT_MMKV_CLIENT_H_
#define _MMKV_CLIENT_MMKV_CLIENT_H_

#include "mmkv/protocol/mmbp_codec.h"
#include "mmkv/protocol/mmbp_response.h"
#include "response_printer.h"

#include <kanon/net/user_client.h>
#include <kanon/thread/condition.h>
#include <kanon/thread/mutex_lock.h>
#include <kanon/util/noncopyable.h>

struct Replxx;

namespace mmkv {
namespace client {

class MmkvClient {
  DISABLE_EVIL_COPYABLE(MmkvClient)
  
 public:
  MmkvClient(EventLoop* loop, InetAddr const& server_addr);
  ~MmkvClient() noexcept;

  void Start();
  
  /**
   * \return
   *   true -- 需要等待
   */
  bool ConsoleIoProcess();
  void IoWait() {
    io_cond_.Wait();
  }
  
  KANON_INLINE Replxx *replxx() KANON_NOEXCEPT { return replxx_; }
 private:
  void InstallLinenoise() KANON_NOEXCEPT;   

  TcpClientPtr client_;
  protocol::MmbpCodec codec_;

  ResponsePrinter response_printer_;
  kanon::Condition io_cond_;
  kanon::MutexLock mutex_;
  
  std::string prompt_;
  
  Replxx *replxx_;
  protocol::Command current_cmd_;
};

} // client
} // mmkv

#endif // _MMKV_CLIENT_MMKV_CLIENT_H_
