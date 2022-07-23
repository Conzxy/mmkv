#include "mmkv_client.h"

#include <iostream>

#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/util/macro.h"

#include "response_printer.h"
#include "translator.h"
#include "information.h"

#include "third-party/linenoise/linenoise.h"
// #include "linenoise.h"

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

  codec_.SetMessageCallback([this](TcpConnectionPtr const&, Buffer& buffer, uint32_t, TimeStamp) {
    MmbpResponse response;
    response.ParseFrom(buffer);
    // std::cout << response->GetContent() << "\n";
    response_printer_.Printf(current_cmd_, &response);

    // ConsoleIoProcess();
    io_cond_.Notify();
  });
}

MmkvClient::~MmkvClient() noexcept {

}

bool MmkvClient::ConsoleIoProcess() {
  std::string statement; 

  std::cout << "mmkv " << client_.GetServerAddr().ToIpPort() << "> ";
  ::linenoise("")
  getline(std::cin, statement);
  MmbpRequest request;
  
  Translator translator;
  auto error_code = translator.Parse(&request, statement); 
  current_cmd_ = (Command)request.command;

  switch (error_code) {
    case Translator::E_INVALID_COMMAND: {
      // 如果用户多次输入错误，可能导致递归炸栈，因此这里避免递归调用
      // ConsoleIoProcess();
      return false;
    }
      break;
    
    case Translator::E_EXIT: {
      is_exit = true;
      client_.Disconnect();
    }
      break;

    case Translator::E_SYNTAX_ERROR: {
      std::cout << "Syntax error: " << command_hints[request.command] << std::endl;
      // ConsoleIoProcess();
      return false;
    }
      break;
    
    case Translator::E_NO_COMMAND: {
      std::cout << "Syntax error: no command\n";
      // std::cout << HELP_INFORMATION;
      return false;
    }
      break;

    case Translator::E_OK: {
      codec_.Send(client_.GetConnection(), &request);
    }
      break;

    default:
      ;
  }
  
  return true;
}

void MmkvClient::Start() {
  std::cout << APPLICATION_INFORMATION << "\n" << std::endl;
  std::cout << "Connecting " << client_.GetServerAddr().ToIpPort() << " ...\n";
  
  client_.Connect();
}
