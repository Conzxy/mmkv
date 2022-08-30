#ifndef MMKV_TRACKER_OBSERVER_H_
#define MMKV_TRACKER_OBSERVER_H_

#include <deque>
#include <kanon/net/user_client.h>
#include <kanon/thread/count_down_latch.h>

#include "configuration.h"
#include "mmkv/protocol/mmbp_codec.h"

namespace mmkv {

using kanon::CountDownLatch;
using mmkv::protocol::MmbpCodec;

/**
 * Used for debugging.
 * Query the configuration of the cluster
 */
class TrackerObserver : kanon::noncopyable {
 public:
  TrackerObserver(EventLoop *loop, InetAddr const &tracker_addr);

  /**
   * Query the configuartion of cluster
   * \note asynchronously
   */
  void Query(Configuration &config);

  void Wait()
  {
    latch_.Wait();
  }

  void Connect()
  {
    cli_->Connect();
    latch_.Wait();
  }

  void DisConnect()
  {
    cli_->Disconnect();
  }

 private:
  TcpClientPtr cli_;

  MmbpCodec codec_;
  CountDownLatch latch_;
  std::deque<void *> configs_;
};

} // namespace mmkv

#endif
