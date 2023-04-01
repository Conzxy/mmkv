#include "mmkv_client.h"

#include <inttypes.h>

#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"

#include "mmkv/util/macro.h"
#include "mmkv/util/print_util.h"
#include "mmkv/util/str_util.h"
#include "mmkv/util/time_util.h"

#include "information.h"
#include "option.h"
#include "response_printer.h"
#include "translator.h"

#include <kanon/string/string_util.h>
#include <kanon/net/tcp_client.h>
#include <ternary_tree.h>
#include <third-party/replxx/include/replxx.h>

#include <kanon/net/callback.h>
#include <kanon/util/ptr.h>

using namespace mmkv::protocol;
using namespace mmkv::client;
using namespace std;
using namespace kanon;

/* Command is "quit/exit" ==> true */
static bool is_exit = false;

#define COMMAND_HISTORY_LOCATION "./.mmkv-command.history"

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

MmkvClient::MmkvClient(EventLoop *loop, InetAddr const &server_addr)
  : client_(NewTcpClient(loop, server_addr, "Mmkv console client"))
  , codec_(MmbpResponse::GetPrototype())
  , io_cond_(mutex_)
  , replxx_(replxx_init())
{
  client_->SetConnectionCallback([this,
                                  &server_addr](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      codec_.SetUpConnection(conn);
      printf("Connect to %s successfully\n\n", server_addr.ToIpPort().c_str());
      // ConsoleIoProcess();
      io_cond_.Notify();

      if (prompt_.empty())
        ::mmkv::util::StrCat(prompt_, YELLOW "mmkv %a> " RESET,
               server_addr.ToIpPort().c_str());
    } else {
      if (is_exit) {
        puts("Disconnect successfully!");
        ::exit(0);
      } else {
        puts("\nConnection is closed by peer server");
        if (!cli_option().reconnect) ::exit(0);
      }
    }
  });

  codec_.SetErrorCallback(
      [](TcpConnectionPtr const &conn, MmbpCodec::ErrorCode code) {
        MMKV_UNUSED(conn);
        util::ErrorPrintf("ERROR occurred: %s", MmbpCodec::GetErrorString(code));
      });

  codec_.SetMessageCallback([this](TcpConnectionPtr const &, Buffer &buffer,
                                   uint32_t, TimeStamp recv_time) {
    MmbpResponse response;
    response.ParseFrom(buffer);
    // std::cout << response->GetContent() << "\n";
    response_printer_.Printf(current_cmd_, &response);
    ::printf("(%.3lf sec)\n",
             (double)(recv_time.GetMicrosecondsSinceEpoch() - start_time) /
                 1000000);

    // ConsoleIoProcess();
    io_cond_.Notify();
  });

  InstallLinenoise();
  if (cli_option().reconnect) client_->EnableRetry();
}

MmkvClient::~MmkvClient() noexcept
{
  ternary_free(&command_tst);
  replxx_end(replxx_);
}

bool MmkvClient::ConsoleIoProcess()
{
  // line is managed by replxx
  auto line = ::replxx_input(replxx_, prompt_.c_str());
  if (!line) {
    return false;
  }

  if (::strcasecmp(line, "clear") == 0) {
    replxx_clear_screen(replxx_);
    return false;
  }

  ::replxx_history_add(replxx_, line);
  if (::replxx_history_save(replxx_, COMMAND_HISTORY_LOCATION) < 0) {
    ::fprintf(stderr, "Failed to save the command history\n");
  }
  // std::string statement;
  // getline(std::cin, statement);
  MmbpRequest request;

  Translator translator;
  auto error_code = translator.Parse(&request, line);
  current_cmd_ = (Command)request.command;

  switch (error_code) {
    case Translator::E_INVALID_COMMAND:
    {
      // 如果用户多次输入错误，可能导致递归炸栈，因此这里避免递归调用
      // ConsoleIoProcess();
      return false;
    } break;

    case Translator::E_EXIT:
    {
      is_exit = true;
      client_->Disconnect();
    } break;

    case Translator::E_SYNTAX_ERROR:
    {
      util::ErrorPrintf("Syntax error: %s\n",
                  GetCommandHint((Command)request.command).c_str());
      // ConsoleIoProcess();
      return false;
    } break;

    case Translator::E_NO_COMMAND:
    {
      util::ErrorPrintf("Syntax error: no command\n");
      // std::cout << GetHelp();
      return false;
    } break;

    case Translator::E_OK:
    {
      codec_.Send(client_->GetConnection(), &request);
      start_time = util::GetTimeUs();
    } break;

    case Translator::E_SHELL_CMD:
    {
      return false;
    } break;
    default:;
  }

  return true;
}

void MmkvClient::Start()
{
  printf("%s\n\n", APPLICATION_INFORMATION);
  printf("Connecting %s...\n", client_->GetServerAddr().ToIpPort().c_str());

  client_->Connect();
}

/* Callback of command hint */
static void OnCommandHint(char const *line, replxx_hints *lc, int *context_len,
                          ReplxxColor *color, void *args)
{
  *color = ReplxxColor::REPLXX_COLOR_YELLOW;
  const size_t len = strlen(line);
  StringView line_view(line, len);
  auto blank_pos = line_view.find(' ');

  /* Although command isn't complete, we also should provide
     hint message */
  if (blank_pos == StringView::npos)
    blank_pos = len - 1;
  
  const size_t cmd_len = blank_pos + 1;
  char command_buf[cmd_len+1];
  for (size_t i = 0; i < cmd_len; ++i) {
    command_buf[i] =
        (line[i] <= 'z' && line[i] >= 'a') ? line[i] - 0x20 : line[i];
  }
  command_buf[cmd_len] = 0;

  /* Must update context_len, since replxx reset context_len = 0
     when user type space key default.

     context_len is the length of displaying message.
     (hint's length - context_len) is the gray hint message length.
     If context_len = 0, replxx think there is no displaying line
     and this callback show the complete hint.
     e.g.
     stradd (stradd key value) <-hint also includes stradd!
            |
           cursor(context_len = 0, replxx think no displaying message)

     Set the context_len to current length of line can fix.
 */
  *context_len = len;
  auto cmd_strings = GetCommandStrings();
  auto cmd_hints = GetCommandHints();
  auto cmd_count = Command::COMMAND_NUM;
  std::string hint; /* reserve space */
  
  /* Although command isn't complete, we also should provide
     hint message */
  for (size_t i = 0; i < cmd_count; ++i) {
    auto cmd_str = cmd_strings[i];
    auto cmd_hint = cmd_hints[i];
    if (len > cmd_str.size()) continue;
    if (!::memcmp(cmd_str.c_str(), command_buf, cmd_len)) {
      hint.clear();
      kanon::StrAppend(hint, cmd_str, cmd_hint);
      ::replxx_add_hint(lc, hint.c_str());
    }
  }
}

static KANON_DEPRECATED_ATTR void OnCommandModify(char **p_line, int *cursor_pos, void *args)
{
  auto line = *p_line;
  const size_t len = strlen(line);
  char command_buf[MAX_LINE];
  Command cmd;

  for (size_t i = 0; i < len; ++i) {
    command_buf[i] =
        (line[i] <= 'z' && line[i] >= 'a') ? line[i] - 0x20 : line[i];
  }
  command_buf[len] = 0;
  if ((cmd = GetCommand({command_buf, len})) != Command::COMMAND_NUM) {
    auto old_line = *p_line;
    auto new_line = ::kanon::StrCat(line, GetCommandHint(cmd));
    *p_line = (char*)calloc(new_line.size() + 1, 1);
    ::strncpy(*p_line, new_line.c_str(), new_line.size());
    ::free(old_line);
  }
}

/* Callback of ternary_search_prefix */
static KANON_INLINE void AddCompletion(char const *cmd, void *args)
{
  auto *lc = (replxx_completions *)args;
  replxx_add_completion(lc, cmd);
}

/* Callback of command completion */
static void OnCommandCompletion(char const *line, replxx_completions *lc,
                                int *context_len, void *args)
{
  static char line_lower[4096];
  strcpy(line_lower, line);
  for (auto &c : line_lower) {
    if (c >= 'a' && c <= 'z') c -= 0x20;
  }

  ternary_search_prefix(command_tst, line_lower, &AddCompletion, lc);
}

void MmkvClient::InstallLinenoise() KANON_NOEXCEPT
{
  ::replxx_install_window_change_handler(replxx_);
  for (uint16_t i = 0; i < COMMAND_NUM; ++i) {
    ternary_add(&command_tst, GetCommandString((Command)i).c_str(), false);
  }

  ternary_add(&command_tst, "EXIT", false);
  ternary_add(&command_tst, "QUIT", false);
  ternary_add(&command_tst, "HELP", false);

  if (::replxx_history_load(replxx_, COMMAND_HISTORY_LOCATION) < 0) {
    ::fprintf(stderr, "Failed to load the command history\n"
                      "OR there is no command history");
  }

  ::replxx_set_indent_multiline(replxx_, prompt_.size());
  ::replxx_set_max_history_size(replxx_, 10);
  ::replxx_set_completion_callback(replxx_, &OnCommandCompletion, nullptr);
  ::replxx_set_hint_callback(replxx_, &OnCommandHint, nullptr);
  ::replxx_set_max_hint_rows(replxx_, COMMAND_NUM/5);
  //::replxx_set_modify_callback(replxx_, &OnCommandModify, nullptr);
}
