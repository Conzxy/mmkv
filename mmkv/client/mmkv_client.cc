#include "mmkv_client.h"

#include <inttypes.h>

#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"

#include "mmkv/util/macro.h"
#include "mmkv/util/str_util.h"
#include "mmkv/util/time_util.h"
#include "mmkv/util/print_util.h"

#include "response_printer.h"
#include "translator.h"
#include "information.h"
#include "option.h"

#include <kanon/net/tcp_client.h>
#include <linenoise.h>
#include <ternary_tree.h>

#include <kanon/net/callback.h>
#include <kanon/util/ptr.h>


using namespace mmkv::protocol;
using namespace mmkv::client;
using namespace mmkv::util;
using namespace std;
using namespace kanon;

/* Command is "quit/exit" ==> true */
static bool is_exit = false;

#define COMMAND_HISTORY_LOCATION "/tmp/.mmkv-command.history"
#ifdef MAX_LINE
#undef MAX_LINE
#endif
#define MAX_LINE 4096

/* Record the start time when send request */
static int64_t start_time = 0;
/* Store the commands and support fast prefix search */
static TernaryNode *command_tst = nullptr;
/* Set config of linenoise */
static void InstallLinenoise();

MmkvClient::MmkvClient(EventLoop* loop, InetAddr const& server_addr) 
  : client_(NewTcpClient(loop, server_addr, "Mmkv console client"))
  , codec_(MmbpResponse::GetPrototype())
  , io_cond_(mutex_)
{
  client_->SetConnectionCallback([this, &server_addr](TcpConnectionPtr const& conn) {
    if (conn->IsConnected()) {
      codec_.SetUpConnection(conn);
      printf("Connect to %s successfully\n\n", server_addr.ToIpPort().c_str());
      // ConsoleIoProcess();
      io_cond_.Notify();

      if (prompt_.empty()) StrCat(prompt_, "mmkv %a> ", server_addr.ToIpPort().c_str());
    } else {
      if (is_exit) {
        puts("Disconnect successfully!");
        ::exit(0);
      } else {
        puts("\nConnection is closed by peer server");
        if (!cli_option().reconnect)
          ::exit(0);
      }

    }
  });
  
  codec_.SetErrorCallback([](TcpConnectionPtr const& conn, MmbpCodec::ErrorCode code) {
    MMKV_UNUSED(conn);
    ErrorPrintf("ERROR occurred: %s",  MmbpCodec::GetErrorString(code));
  });

  codec_.SetMessageCallback([this](TcpConnectionPtr const&, Buffer& buffer, uint32_t, TimeStamp recv_time) {
    MmbpResponse response;
    response.ParseFrom(buffer);
    // std::cout << response->GetContent() << "\n";
    response_printer_.Printf(current_cmd_, &response);
    ::printf("(%.3lf sec)\n", (double)(recv_time.GetMicrosecondsSinceEpoch() - start_time) / 1000000);
    
    // ConsoleIoProcess();
    io_cond_.Notify();
  });

  InstallLinenoise();
  if (cli_option().reconnect)
    client_->EnableRetry();
}

MmkvClient::~MmkvClient() noexcept {
  ternary_free(&command_tst);
}

bool MmkvClient::ConsoleIoProcess() {
  auto line = ::linenoise(prompt_.c_str());
  if (!line) {
    return false;
  }

  std::unique_ptr<char> wrapper_line(line);
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
      client_->Disconnect();
    }
      break;

    case Translator::E_SYNTAX_ERROR: {
      ErrorPrintf("Syntax error: %s\n", GetCommandHint((Command)request.command).c_str());
      // ConsoleIoProcess();
      return false;
    }
      break;
    
    case Translator::E_NO_COMMAND: {
      ErrorPrintf("Syntax error: no command\n");
      // std::cout << GetHelp();
      return false;
    }
      break;

    case Translator::E_OK: {
      codec_.Send(client_->GetConnection(), &request);
      start_time = GetTimeUs();
    }
      break;

    case Translator::E_SHELL_CMD: {
      return false;
    }
      break;
    default:
      ;
  }
  
  return true;
}

void MmkvClient::Start() {
  printf("%s\n\n", APPLICATION_INFORMATION);
  printf("Connecting %s...\n", client_->GetServerAddr().ToIpPort().c_str());
  
  client_->Connect();
}

/* Callback of command hint */
static inline char *OnCommandHint(char const *line, int *color, int *bold) {
  *color = 35;
  *bold = 0;
  const size_t len = strlen(line);
  char command_buf[MAX_LINE];
  Command cmd;

  for (size_t i = 0; i < len; ++i) {
    command_buf[i] = (line[i] <= 'z' && line[i] >= 'a') ? line[i] - 0x20 : line[i];
  } 
  command_buf[len] = 0;
  if ((cmd = GetCommand({command_buf, len})) != Command::COMMAND_NUM) {
    return (char*)GetCommandHint(cmd).c_str();
  }
  return NULL;
}

/* Callback of ternary_search_prefix */
static inline void AddCompletion(char const *cmd, void *args) {
  linenoiseCompletions *lc = (linenoiseCompletions*)args;
  linenoiseAddCompletion(lc, cmd);
}

/* Callback of command completion */
static void OnCommandCompletion(char const *line, linenoiseCompletions *lc) {
  static char line_lower[4096];
  strcpy(line_lower, line);
  for (auto &c : line_lower) {
    if (c >= 'a' && c <= 'z')
      c -= 0x20;
  }
  
  ternary_search_prefix(command_tst, line_lower, &AddCompletion, lc);
}

static inline void InstallLinenoise() {
  for (uint16_t i = 0; i < COMMAND_NUM; ++i) {
    ternary_add(&command_tst, GetCommandString((Command)i).c_str(), false);
  }

  ternary_add(&command_tst, "EXIT", false);
  ternary_add(&command_tst, "QUIT", false);
  ternary_add(&command_tst, "HELP", false);

  if (::linenoiseHistoryLoad(COMMAND_HISTORY_LOCATION) < 0) {
    ::fprintf(stderr, "Failed to load the command history\n");
    ::exit(0);
  }

  ::linenoiseSetMultiLine(1); 
  ::linenoiseHistorySetMaxLen(10);
  ::linenoiseSetCompletionCallback(&OnCommandCompletion);
  ::linenoiseSetHintsCallback(&OnCommandHint);
}
