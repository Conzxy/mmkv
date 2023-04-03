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

#include <kanon/net/tcp_client.h>
#include <kanon/string/string_util.h>
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

  codec_.SetErrorCallback([](TcpConnectionPtr const &conn,
                             MmbpCodec::ErrorCode code) {
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

bool MmkvClient::CliCommandProcess(kanon::StringView cmd,
                                   kanon::StringView line)
{
  // Check if line is a cli command
  auto cli_cmd = GetCliCommand(cmd);
  if (cli_cmd != CliCommand::CLI_COMMAND_NUM) {
    switch (cli_cmd) {
      case CLI_HELP:
        fwrite(GetHelp().c_str(), 1, GetHelp().size(), stdout);
        break;
      case CLI_EXIT:
      case CLI_QUIT:
      {
        is_exit = true;
        client_->Disconnect();
      } break;
      case CLI_HISTORY:
      {
        const auto history_sz = replxx_history_size(replxx_);
        int show_cnt = 20;
        if (history_sz < show_cnt) {
          show_cnt = history_sz;
        }

        auto *history_scan = replxx_history_scan_start(replxx_);
        ReplxxHistoryEntry history_entry;
        for (int i = 0; replxx_history_scan_next(replxx_, history_scan,
                                                 &history_entry) == 0;
             ++i)
        {
          printf("%*d: %s\n",
                 (int)(prompt_.size() - (sizeof(YELLOW) + sizeof(RESET) - 2)),
                 i, history_entry.text);
          if (i == show_cnt) break;
        }
      } break;

      case CLI_CLEAR:
      {
        replxx_clear_screen(replxx_);
      } break;

      default:
        LOG_FATAL << "Don't implement the handler of "
                  << GetCliCommandString(cli_cmd);
    }

    return false;
  }
  return true;
}

bool MmkvClient::ShellCommandProcess(kanon::StringView cmd,
                                     kanon::StringView line)
{
  // Check if line is a shell cmd
  if (cmd.size() > 0 && cmd[0] == '!') {
    if (::system(line.substr(1).data())) {
      util::ErrorPrintf("Syntax error: no command\n");
    }
    return false;
  }
  return true;
}

int MmkvClient::MmkvCommandProcess(kanon::StringView cmd,
                                   kanon::StringView line)
{
  // Check if line is a mmkv command
  auto mmkv_cmd = GetCommand(cmd);
  if (mmkv_cmd == Command::COMMAND_NUM) {
    return Translator::E_NO_COMMAND;
  }
  MmbpRequest request;
  Translator translator;

  // const auto first_token_pos = line.find_first_not_of(' ', cmd.size());
  // line.remove_prefix(std::min(first_token_pos, line.size()));

  line.remove_prefix(cmd.size());
  LOG_DEBUG << "Mmkv token string: " << line;
  auto error_code = translator.Parse(&request, mmkv_cmd, line);
  current_cmd_ = (Command)request.command;

  switch (error_code) {
    case Translator::E_SYNTAX_ERROR:
    {
      return Translator::E_SYNTAX_ERROR;
    } break;

    case Translator::E_OK:
    {
      codec_.Send(client_->GetConnection(), &request);
      start_time = util::GetTimeUs();
    } break;

    default:
      LOG_FATAL << "Unknown error code of Translator";
  }

  return Translator::E_OK;
}

bool MmkvClient::ConsoleIoProcess()
{
  // line is managed by replxx
  auto line = ::replxx_input(replxx_, prompt_.c_str());
  const auto line_len = strlen(line);
  if (!line) {
    return false;
  }

  if (line_len == 0) {
    util::ErrorPrintf("Syntax error: no command\n");
    return false;
  }
  // To any input line,
  // we save it in case user
  // to reuse line although it is invalid.
  ::replxx_history_add(replxx_, line);
  if (::replxx_history_save(replxx_, COMMAND_HISTORY_LOCATION) < 0) {
    ::fprintf(stderr, "Failed to save the command history\n");
  }

  StringView line_view(line, line_len);
  auto space_pos = line_view.find(' ');
  // space_pos is npos is also ok.
  auto cmd = line_view.substr(0, space_pos).ToString();
  auto upper_cmd = line_view.substr(0, space_pos).ToUpperString();

  if (!CliCommandProcess(upper_cmd, line_view)) {
    return false;
  }

  if (!ShellCommandProcess(cmd, line_view)) {
    return false;
  }
  auto err_code = MmkvCommandProcess(upper_cmd, line_view);
  switch (err_code) {
    case Translator::E_OK:
      return true;
    case Translator::E_NO_COMMAND:
    {
      util::ErrorPrintf("ERROR: invalid command: %s\n", cmd.c_str());
    } break;
    case Translator::E_SYNTAX_ERROR:
    {
      util::ErrorPrintf("Syntax error: %s%s\n", cmd.c_str(),
                        GetCommandHint(GetCommand(cmd)).c_str());

    } break;
  }
  return false;
}

void MmkvClient::Start()
{
  printf("%s\n\n", APPLICATION_INFORMATION);
  printf("Connecting %s...\n", client_->GetServerAddr().ToIpPort().c_str());

  client_->Connect();
}

/*--------------------------------------------------*/
/* Command linenoise install(replxx)                */
/*--------------------------------------------------*/

KANON_INLINE void RegisterCommandHints(size_t cmd_count,
                                       std::string const *cmd_strings,
                                       std::string const *cmd_hints,
                                       size_t line_len, char const *command_buf,
                                       size_t cmd_len, std::string &hint,
                                       replxx_hints *lc) KANON_NOEXCEPT
{
  for (size_t i = 0; i < cmd_count; ++i) {
    auto cmd_str = cmd_strings[i];
    auto cmd_hint = cmd_hints[i];
    if (line_len > cmd_str.size()) continue;
    /* Although command isn't complete, we also should provide
       hint message */
    if (!::memcmp(cmd_str.c_str(), command_buf, cmd_len)) {
      hint.clear();
      kanon::StrAppend(hint, cmd_str, cmd_hint);
      ::replxx_add_hint(lc, hint.c_str());
    }
  }
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
  if (blank_pos == StringView::npos) blank_pos = len - 1;

  const size_t cmd_len = blank_pos + 1;
  char command_buf[cmd_len + 1];
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
  auto cli_cmd_hints = GetCliCommandHints();
  auto cli_cmd_count = CliCommand::CLI_COMMAND_NUM;
  auto cli_cmd_strings = GetCliCommandStrings();
  std::string hint; /* reserve space */

  RegisterCommandHints(cmd_count, cmd_strings, cmd_hints, len, command_buf,
                       cmd_len, hint, lc);
  RegisterCommandHints(cli_cmd_count, cli_cmd_strings, cli_cmd_hints, len,
                       command_buf, cmd_len, hint, lc);
}

static KANON_DEPRECATED_ATTR void OnCommandModify(char **p_line,
                                                  int *cursor_pos, void *args)
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
    *p_line = (char *)calloc(new_line.size() + 1, 1);
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

  for (size_t i = 0; i < CLI_COMMAND_NUM; ++i) {
    ternary_add(&command_tst, GetCliCommandString((CliCommand)i).c_str(),
                false);
  }

  if (::replxx_history_load(replxx_, COMMAND_HISTORY_LOCATION) < 0) {
    ::fprintf(stderr, "Failed to load the command history\n"
                      "OR there is no command history");
  }

  ::replxx_set_indent_multiline(replxx_, prompt_.size());
  ::replxx_set_max_history_size(replxx_, 10);
  ::replxx_set_completion_callback(replxx_, &OnCommandCompletion, nullptr);
  ::replxx_set_hint_callback(replxx_, &OnCommandHint, nullptr);
  ::replxx_set_max_hint_rows(replxx_, COMMAND_NUM / 5);
  //::replxx_set_modify_callback(replxx_, &OnCommandModify, nullptr);
}
