#include "mmkv_server.h"

#include "mmkv_session.h"

#include "mmkv/disk/recover.h"
#include "mmkv/disk/request_log.h"

using namespace kanon;
using namespace mmkv::server;
using namespace mmkv::disk;

MmkvServer::MmkvServer(EventLoop* loop, InetAddr const& addr)
  : server_(loop, addr, "In-Memory Key-Value database server")
{
  server_.SetConnectionCallback([this](TcpConnectionPtr const& conn) {
    if (conn->IsConnected()) {
      conn->SetContext(new MmkvSession(conn, this));
    } else {
      auto p = AnyCast<MmkvSession*>(conn->GetContext());
      assert(p);

      delete *p;
    }
  });
}

MmkvServer::~MmkvServer() noexcept {

}

void MmkvServer::Start() {
  g_log_request = true;
  if (g_log_request) {
    LOG_INFO << "Recover from request log";
    Recover recover;
    recover.ParseFromRequest();
    LOG_INFO << "Recover complete";
  }

  g_rlog.Start();
  Listen();
}