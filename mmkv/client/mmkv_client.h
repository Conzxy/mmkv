// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_CLIENT_MMKV_CLIENT_H_
#define _MMKV_CLIENT_MMKV_CLIENT_H_

#include "mmkv/protocol/mmbp_codec.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/configd/configd_client.h"
#include "response_printer.h"

#include <kanon/net/user_client.h>
#include <kanon/thread/condition.h>
#include <kanon/thread/mutex_lock.h>
#include <kanon/thread/count_down_latch.h>
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

  void IoWait();
  void ConnectWait();

 private:
  KANON_INLINE void SetupMmkvClient(InetAddr const &addr);
  KANON_INLINE void SetupConfigClient();

  KANON_INLINE bool CliCommandProcess(kanon::StringView cmd, kanon::StringView line);
  KANON_INLINE bool ShellCommandProcess(kanon::StringView line);

  KANON_INLINE bool ConfigCommandProcess(kanon::StringView cmd, kanon::StringView line);
  KANON_INLINE void MmkvCommandProcess(kanon::StringView cmd, kanon::StringView line);
  KANON_INLINE void SelectNode(InetAddr const &node_addr, node_id_t node_id);
  KANON_INLINE void SetPrompt();

  void InstallLinenoise() KANON_NOEXCEPT;

  TcpClientPtr        client_;
  protocol::MmbpCodec codec_;

  ResponsePrinter       response_printer_;
  kanon::MutexLock      mutex_;
  kanon::CountDownLatch io_latch_;

  std::string prompt_;

  Replxx           *replxx_;
  protocol::Command current_cmd_;

  std::unique_ptr<ConfigdClient> p_conf_cli_;

  int        wait_cli_num_;
  EventLoop *p_loop_;
  node_id_t  current_peer_node_id = -1;

  std::string cur_cmd_;
  std::string upper_cur_cmd_;
};

} // namespace client
} // namespace mmkv

#endif // _MMKV_CLIENT_MMKV_CLIENT_H_
