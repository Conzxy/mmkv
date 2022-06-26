#include "mmkv_client.h"

#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/protocol/translator.h"
#include "mmkv/util/macro.h"

#include "information.h"

#include <iostream>
#include <kanon/net/callback.h>
#include <kanon/util/ptr.h>

using namespace mmkv::protocol;
using namespace mmkv::client;
using namespace std;
using namespace kanon;

static bool is_exit = false;

MmkvClient::MmkvClient(EventLoop* loop, InetAddr const& server_addr) 
  : client_(loop, server_addr, "Mmkv console client") 
  , codec_(MmbpResponse::GetPrototype())
  , io_cond_(mutex_)
{
  client_.SetConnectionCallback([this, &server_addr](TcpConnectionPtr const& conn) {
    if (conn->IsConnected()) {
      codec_.SetUpConnection(conn);
      std::cout << "Connect to " << server_addr.ToIpPort() << " successfully\n\n";
      // ConsoleIoProcess();
      io_cond_.Notify();
    } else {
      if (is_exit) {
        std::cout << "Disconnect successfully!" << std::endl;
      } else {
        std::cout << "\nConnection is closed by peer server" << std::endl;
      }

      ::exit(0);
    }
  });
  
  codec_.SetErrorCallback([](TcpConnectionPtr const& conn, MmbpCodec::ErrorCode code) {
    MMKV_UNUSED(conn);
    std::cout << "ERROR occurred: " << MmbpCodec::GetErrorString(code);
  });

  codec_.SetMessageCallback([this](TcpConnectionPtr const& conn, std::unique_ptr<MmbpMessage> msg, TimeStamp recv_time) {
    MMKV_UNUSED(recv_time);
    MMKV_UNUSED(conn);
    auto response = kanon::down_pointer_cast<MmbpResponse>(msg);

    std::cout << response->GetContent() << "\n";

    // ConsoleIoProcess();
    io_cond_.Notify();
  });
}

MmkvClient::~MmkvClient() noexcept {

}

void MmkvClient::ConsoleIoProcess() {
  vector<String> args;
  args.resize(4);
  std::string arg;
  
  std::cout << "Mmkv " << client_.GetServerAddr().ToIpPort() << " >> ";
  std::cin >> args[0];

  auto const& cmd = args[0];
  
  MmbpRequest request;

  if (cmd == "stradd") {
    std::cin >> args[1] >> args[2];
    request.SetCommand(STR_ADD);
    request.SetKey(args[1]);
    request.SetValue(args[2]);
  } else if (cmd == "strget") {
    std::cin >> args[1];
    request.SetCommand(STR_GET);
    request.SetKey(args[1]);
  } else if (cmd == "strset") {
    std::cin >> args[1] >> args[2];
    request.SetCommand(STR_SET);
    request.SetKey(args[1]);
    request.SetValue(args[2]);
  } else if (cmd == "strdel") {
    std::cin >> args[1];
    request.SetCommand(STR_DEL);
    request.SetKey(args[1]);
  } else if (cmd == "memory_stat") {
    request.SetCommand(MEM_STAT);
  } else if (cmd == "exit") {
    is_exit = true;
    client_.Disconnect();
  } else if (cmd == "help") {
    std::cout << HELP_INFORMATION << std::endl;
    ConsoleIoProcess();
    return;
  } else {
    std::cout << "ERROR: invalid command: " << cmd << "\n";
    std::cout << HELP_INFORMATION << std::endl;
    ConsoleIoProcess();
    return;
  }
  
  codec_.Send(client_.GetConnection(), &request);
}

void MmkvClient::Start() {
  std::cout << APPLICATION_INFORMATION << "\n" << std::endl;
  std::cout << "Connecting " << client_.GetServerAddr().ToIpPort() << " ...\n";
  
  client_.Connect();
}
