#include "mmkv_client.h"

#include <iostream>

#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"

#include "mmkv/util/macro.h"
#include "mmkv/util/str_util.h"

#include "response_printer.h"
#include "translator.h"
#include "information.h"

#include "linenoise.h"
// #include "linenoise.h"

#include <kanon/net/callback.h>
#include <kanon/util/ptr.h>


using namespace mmkv::protocol;
using namespace mmkv::client;
using namespace mmkv::util;
using namespace std;
using namespace kanon;

static bool is_exit = false;

#define COMMAND_HISTORY_LOCATION "/tmp/.mmkv-command.history"

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

      if (prompt_.empty()) StrCat(prompt_, "mmkv %a> ", server_addr.ToIpPort().c_str());
    } else {
      if (is_exit) {
        std::cout << "Disconnect successfully!" << std::endl;
        ::exit(0);
      } else {
        printf("sssss\n");
        printf("xxxxxx\n");
        std::cout << "\nConnection is closed by peer server" << std::endl;
      }

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

  InstallLinenoise();
  client_.EnableRetry();
}

MmkvClient::~MmkvClient() noexcept {
}

static char *InstallCommandHints(char const *line, int *color, int *bold) {
  *color = 36;
  *bold = 0;
  for (int i = 0; i < COMMAND_NUM; ++i) {
    if (!strcasecmp(line, GetCommandString((Command)i).c_str())) {
      return (char*)GetCommandHint((Command)i).c_str();
    }
  }

  return NULL;
}

static void InstallCommandCompletion(char const *line, linenoiseCompletions *lc) {
#define _REGISTER_COMMAND_COMPLETION_CH2(_ch, _cmd) \
  if (line[0] == _ch) \
    ::linenoiseAddCompletion(lc, _cmd);

  StringView line_view(line);
  for (int n = line_view.size(); n >= 1; --n) {
    for (int i = 0; i < COMMAND_NUM; ++i) {
      StringView command(GetCommandString((Command)i));

      if (!::strncasecmp(line_view.data(), command.data(), n)) {
        ::linenoiseAddCompletion(lc, command.data());
      }
    }
  }

  _REGISTER_COMMAND_COMPLETION_CH2('h', "help")
  _REGISTER_COMMAND_COMPLETION_CH2('q', "quit")
  _REGISTER_COMMAND_COMPLETION_CH2('e', "exit")
}

void MmkvClient::InstallLinenoise() {
  if (::linenoiseHistoryLoad(COMMAND_HISTORY_LOCATION) < 0) {
    ::fprintf(stderr, "Failed to load the command history\n");
    ::exit(0);
  }

  ::linenoiseSetMultiLine(1); 
  ::linenoiseHistorySetMaxLen(10);
  ::linenoiseSetCompletionCallback(&InstallCommandCompletion);
  ::linenoiseSetHintsCallback(&InstallCommandHints);
}

bool MmkvClient::ConsoleIoProcess() {
  auto line = ::linenoise(prompt_.c_str());
  if (!line) {
    return false;
  }

  if (::strcasecmp(line, "clear") == 0) {
    ::linenoiseClearScreen();
    return false;
  }

  linenoiseHistoryAdd(line);

  if (::linenoiseHistorySave(COMMAND_HISTORY_LOCATION) < 0) {
    ::fprintf(stderr, "Failed to save the command history\n");
  }
  // std::string statement; 
  // getline(std::cin, statement);
  MmbpRequest request;
  
  Translator translator;
  auto error_code = translator.Parse(&request, line); 
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
      std::cout << "Syntax error: " << GetCommandHint((Command)request.command) << std::endl;
      // ConsoleIoProcess();
      return false;
    }
      break;
    
    case Translator::E_NO_COMMAND: {
      std::cout << "Syntax error: no command\n";
      // std::cout << GetHelp();
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
