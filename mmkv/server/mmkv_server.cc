// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv_server.h"

#include "mmkv_session.h"
#include "common.h"

#include "mmkv/disk/recover.h"
#include "mmkv/disk/request_log.h"
#include "mmkv/util/time_util.h"

#include "mmkv/storage/db.h"
#include "mmkv/server/config.h"

using namespace kanon;
using namespace mmkv::server;
using namespace mmkv::disk;
using namespace mmkv::server;
using namespace mmkv::storage;
using namespace mmkv::util;

MmkvServer::MmkvServer(EventLoop* loop, InetAddr const& addr, InetAddr const &sharder_addr)
  : server_(loop, addr, "In-Memory Key-Value database server")
  , tracker_cli_loop_thr_(mmkv_config().IsSharder() ? new EventLoopThread("TrackerClient") : nullptr)
  , tracker_cli_(mmkv_config().IsSharder() ? new TrackerClient(tracker_cli_loop_thr_->StartRun(), InetAddr(mmkv_config().config_server_endpoint), sharder_addr) : nullptr)
{
  server_.SetConnectionCallback([this](TcpConnectionPtr const& conn) {
    if (conn->IsConnected()) {
      conn->SetContext(*new MmkvSession(conn, this));
      LOG_MMKV(conn) << " connected";
    } else {
      auto p = AnyCast<MmkvSession>(conn->GetContext());
      assert(p);

      delete p;
      LOG_MMKV(conn) << " disconnected";
    }
  });

  if (tracker_cli_) {
    LOG_INFO << "This is configured as a shard server also";
    LOG_INFO << "Connecting to the router server...";
    tracker_cli_->Connect();
  }
}

MmkvServer::~MmkvServer() noexcept {

}

void MmkvServer::Start() {
  if (mmkv_config().log_method == LM_REQUEST) {
    auto stime = GetTimeMs();
    LOG_INFO << "Recover from request log";
    try { 
      Recover recover;
      recover.ParseFromRequest();
      LOG_INFO << "Recover complete";
    } catch (FileException const &ex) {
      LOG_ERROR << ex.what();
      LOG_ERROR << "Can't recover database from log";
    }
    LOG_INFO << "Recover cost: " << (GetTimeMs() - stime) << "ms";
    rlog().Start();
  }

  if (mmkv_config().expiration_check_cycle > 0) {
    LOG_INFO << "The mmkv will check all expired entries actively";
    LOG_INFO << "The cycle is " << mmkv_config().expiration_check_cycle << " seconds";

    server_.GetLoop()->RunEvery([]() {
      // FIXME thread-safe
      LOG_DEBUG << "Check expiration";
      database_manager().CheckExpirationCycle();
    }, mmkv_config().expiration_check_cycle);
  }

  Listen();
}
