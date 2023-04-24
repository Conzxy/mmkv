// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_CLUSTER_CLUSTER_H_
#define MMKV_CLUSTER_CLUSTER_H_

#include "mmkv/tracker/tracker_observer.h"
#include "mmkv/tracker/tracker.h"
#include "mmkv/tracker/tracker_client.h"

class Cluster {
 public:
  Cluster(EventLoop *loop, InetAddr const &tracker_addr);
  
  void StartShardServer(InetAddr const &sharder_addr);
  void Query(mmkv::Configuration &config);

  void Start();
 private:
  mmkv::server::Tracker tracker_;
  InetAddr tracker_addr_;
  
  EventLoopThread tracker_cli_loop_thr_;
  std::vector<std::unique_ptr<mmkv::server::TrackerClient>> tracker_clis_;
  
  EventLoopThread observer_loop_thr_; 
  mmkv::TrackerObserver observer_;
};

#endif
