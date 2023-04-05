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
  MmkvClient(EventLoop *loop, InetAddr const &server_addr);
  ~MmkvClient() noexcept;

  void Start();

  void ConsoleIoProcess();

  void IoWait()
  {
    KANON_MUTEX_GUARD(mutex_);
    while (need_io_wait_)
      io_cond_.Wait();
  }

  void ConnectWait() { IoWait(); }

 private:
  KANON_INLINE bool CliCommandProcess(kanon::StringView cmd,
                                      kanon::StringView line);
  KANON_INLINE bool ShellCommandProcess(kanon::StringView cmd,
                                        kanon::StringView line);

  KANON_INLINE int MmkvCommandProcess(kanon::StringView cmd,
                                      kanon::StringView line);

  void InstallLinenoise() KANON_NOEXCEPT;

  TcpClientPtr client_;
  protocol::MmbpCodec codec_;

  ResponsePrinter response_printer_;
  kanon::Condition io_cond_;
  kanon::MutexLock mutex_;
  bool need_io_wait_ = true;

  std::string prompt_;

  Replxx *replxx_;
  protocol::Command current_cmd_;
};

} // namespace client
} // namespace mmkv

#endif // _MMKV_CLIENT_MMKV_CLIENT_H_
