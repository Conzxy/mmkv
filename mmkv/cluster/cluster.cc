// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "cluster.h"

using namespace mmkv;
using namespace mmkv::server;

Cluster::Cluster(EventLoop *loop, InetAddr const &tracker_addr)
  : tracker_(loop, tracker_addr)
  , tracker_addr_(tracker_addr)
  , tracker_cli_loop_thr_("TrackerClients")
  , observer_loop_thr_("Observer")
  , observer_(observer_loop_thr_.StartRun(),
              InetAddr("127.0.0.1", tracker_addr.GetPort()))
{
  tracker_cli_loop_thr_.StartRun();
}

void Cluster::Start()
{
  tracker_.Listen();
  observer_.Connect();
}

void Cluster::StartShardServer(InetAddr const &sharder_addr)
{
  tracker_clis_.emplace_back(new TrackerClient(
      tracker_cli_loop_thr_.GetLoop(), tracker_addr_, sharder_addr,
      "TrackerClient", "Sharder" + std::to_string(tracker_clis_.size())));
  tracker_clis_.back()->Connect();
}

void Cluster::Query(mmkv::Configuration &config)
{
  observer_.Query(config);
  observer_.Wait();
}
