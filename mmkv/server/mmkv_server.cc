#include "mmkv_server.h"

#include "mmkv_session.h"
#include "common.h"

#include "mmkv/disk/recover.h"
#include "mmkv/disk/request_log.h"

#include "mmkv/server/config.h"

using namespace kanon;
using namespace mmkv::server;
using namespace mmkv::disk;
using namespace mmkv::server;

MmkvServer::MmkvServer(EventLoop* loop, InetAddr const& addr)
  : server_(loop, addr, "In-Memory Key-Value database server")
{
  server_.SetConnectionCallback([this](TcpConnectionPtr const& conn) {
    if (conn->IsConnected()) {
      conn->SetContext(new MmkvSession(conn, this));
      LOG_MMKV(conn) << " connected";
    } else {
      auto p = AnyCast<MmkvSession*>(conn->GetContext());
      assert(p);

      delete *p;
      LOG_MMKV(conn) << " disconnected";
    }
  });
}

MmkvServer::~MmkvServer() noexcept {

}

void MmkvServer::Start() {
  if (g_config.log_method == LM_REQUEST) {
    LOG_INFO << "Recover from request log";
    Recover recover;
    recover.ParseFromRequest();
    LOG_INFO << "Recover complete";
  }

  g_rlog.Start();
  Listen();
}