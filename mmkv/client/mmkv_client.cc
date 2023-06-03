// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv_client.h"

#include <inttypes.h>

#include "kanon/net/inet_addr.h"
#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/tracker/common_type.h"
#include "mmkv/util/macro.h"
#include "mmkv/util/print_util.h"
#include "mmkv/util/str_util.h"
#include "mmkv/util/time_util.h"
#include "mmkv/util/shard_util.h"
#include "mmkv/util/tokenizer.h"

#include "information.h"
#include "option.h"
#include "response_printer.h"
#include "translator.h"

#include <kanon/net/tcp_client.h>
#include <kanon/string/string_util.h>
#include <kanon/string/string_view_util.h>
#include <ternary_tree.h>
#include <third-party/replxx/include/replxx.h>

#include <kanon/net/callback.h>
#include <kanon/util/ptr.h>

using namespace mmkv::protocol;
using namespace mmkv::client;
using namespace mmkv::util;
using namespace std;
using namespace kanon;
using namespace std::placeholders;

/* Command is "quit/exit" ==> true */
static bool is_exit = false;

#ifdef KANON_ON_UNIX
#  define COMMAND_HISTORY_LOCATION "/tmp/.mmkv-command.history"
#else
#  define COMMAND_HISTORY_LOCATION "./.mmkv-command.history"
#endif

#ifdef MAX_LINE
#  undef MAX_LINE
#endif
#define MAX_LINE 4096

/* Record the start time when send request */
static int64_t      start_time  = 0;
/* Store the commands and support fast prefix search */
static TernaryNode *command_tst = nullptr;

MmkvClient::MmkvClient(EventLoop *loop, InetAddr const &server_addr)
  : client_()
  , codec_(MmbpResponse::GetPrototype())
  , io_latch_()
  , replxx_(replxx_init())
  , wait_cli_num_(0)
  , p_loop_(loop)
{
  // If there are two connection need to connection, wait them
  if (!cli_option().is_conn_configd()) {
    SetupMmkvClient(server_addr);
  } else {
    SetupConfigClient();
  }

  InstallLinenoise();
}

MmkvClient::~MmkvClient() noexcept
{
  ternary_free(&command_tst);
  replxx_end(replxx_);
}

bool MmkvClient::CliCommandProcess(kanon::StringView cmd, kanon::StringView line)
{
  // Check if line is a cli command
  auto cli_cmd = GetCliCommand(cmd);

  if (cli_cmd == CliCommand::CLI_COMMAND_NUM) {
    return false;
  }

  switch (cli_cmd) {
    case CLI_HELP:
      fwrite(GetHelp().c_str(), 1, GetHelp().size(), stdout);
      break;
    case CLI_EXIT:
    case CLI_QUIT: {
      is_exit = true;
      if (client_) client_->Disconnect();
      if (p_conf_cli_) p_conf_cli_->cli_->Disconnect();
    } break;
    case CLI_HISTORY: {
      const auto               history_sz   = replxx_history_size(replxx_);
      auto                    *history_scan = replxx_history_scan_start(replxx_);
      ReplxxHistoryEntry       history_entry;
      std::vector<std::string> history_arr(history_sz);
      for (int i = 0; replxx_history_scan_next(replxx_, history_scan, &history_entry) == 0; ++i) {
        history_arr[i] = history_entry.text;
        if (i == history_sz) break;
      }

      int i = 0;
      for (auto beg = history_arr.rbegin(); beg != history_arr.rend(); ++beg) {
        int align_padding_size = prompt_.size();
        if (client_) {
          align_padding_size -= (sizeof(YELLOW) + sizeof(RESET) - 2);
        }
        if (p_conf_cli_) {
          align_padding_size -= (sizeof(BLUE) + sizeof(RESET) - 2);
        }

        if (i == 0) {
          PrintfAndFlush("%*s: %s\n", align_padding_size, "latest", beg->c_str());
        } else {
          PrintfAndFlush("%*d: %s\n", align_padding_size, i, beg->c_str());
        }
        ++i;
      }
      replxx_history_scan_stop(replxx_, history_scan);
    } break;

    case CLI_CLEAR: {
      replxx_clear_screen(replxx_);
    } break;

    case CLI_CLEAR_HISTORY: {
      // This API just clear the memory history record
      replxx_history_clear(replxx_);

      FILE *file = fopen(COMMAND_HISTORY_LOCATION, "w");
      replxx_history_save(replxx_, COMMAND_HISTORY_LOCATION);
      if (file) fclose(file);
    } break;

    default:
      LOG_FATAL << "Don't implement the handler of " << GetCliCommandString(cli_cmd);
  }
  return true;
}

bool MmkvClient::ShellCommandProcess(kanon::StringView line)
{
  // Check if line is a shell cmd
  if (line.size() > 0 && line[0] == '!') {
#if KANON_ON_WIN
    auto shell_cmd = StrCat("powershell ", line.substr(1));
#elif defined(KANON_ON_UNIX)
    auto shell_cmd = line.substr(1);
#endif
    if (::system(shell_cmd.data())) {
      util::ErrorPrintf("SYNTAX ERROR: no command\n");
    }
    return true;
  }
  return false;
}

void MmkvClient::MmkvCommandProcess(kanon::StringView cmd, kanon::StringView line)
{
  // Check if line is a mmkv command
  auto mmkv_cmd = GetCommand(cmd);
  if (mmkv_cmd == Command::COMMAND_NUM) {
    util::ErrorPrintf("ERROR: invalid command: %s\n", cur_cmd_.c_str());
    return;
  }
  MmbpRequest request;
  Translator  translator;

  // const auto first_token_pos = line.find_first_not_of(' ', cmd.size());
  // line.remove_prefix(std::min(first_token_pos, line.size()));

  line.remove_prefix(cmd.size());
  LOG_DEBUG << "Mmkv token string: " << line;
  auto error_code = translator.Parse(&request, mmkv_cmd, line);
  current_cmd_    = (Command)request.command;

  switch (error_code) {
    case Translator::E_SYNTAX_ERROR: {
      util::ErrorPrintf(
          "SYNTAX ERROR: %s%s\n",
          GetCommandString(GetCommand(upper_cur_cmd_)).c_str(),
          GetCommandHint(GetCommand(upper_cur_cmd_)).c_str()
      );
    } break;

    case Translator::E_OK: {
      // If configd is specified,
      // consider select a suitable node to connect.
      // If the command contains a key, this selection is automatical.
      // If the command has no key, the selection has following cases:
      // - Previous selected node
      // - User specified node
      if (cli_option().is_conn_configd()) {
        ConfigdClient::NodeEndPoint node_ep;
        if (request.HasKey()) {
          if (p_conf_cli_->ShardNum() == 0) {
            util::ErrorPrintf("ERROR: There are no avaliable nodes can execute command!\n");
            return;
          }
          shard_id_t request_shard = MakeShardId(request.key) % p_conf_cli_->ShardNum();
          LOG_INFO << "hash(key) = " << MakeShardId(request.key);
          LOG_INFO << "shard num = " << p_conf_cli_->ShardNum();
          auto ret = p_conf_cli_->QueryNodeEndpoint(request_shard, &node_ep);

          LOG_INFO << "node endpoint = "
                   << "(" << node_ep.node_id << ", " << node_ep.host << ":" << node_ep.port << ")";
          (void)ret;
          assert(ret);

          LOG_INFO << "current peer node id = " << current_peer_node_id;
          LOG_INFO << "node_ep node_id = " << node_ep.node_id;

          SelectNode(InetAddr(node_ep.host, node_ep.port), node_ep.node_id);
        } else {
          util::WarnPrintf("WARN: Command without key don't gather response from all nodes now\n"
                           "ie. The command will get the response from current selected node only\n"
          );
          if (!client_) {
            util::ErrorPrintf("ERROR: There is no selected node\n"
                              "You can select a node by SELECT command\n"
                              "OR a node is selected by command with key\n");
            return;
          }
        }
      }

      codec_.Send(client_->GetConnection(), &request);
      start_time = util::GetTimeUs();
      IoWait();
    } break;

    default:
      LOG_FATAL << "Unknown error code of Translator";
  }
}

void MmkvClient::ConsoleIoProcess()
{
  // line is managed by replxx
  auto       line     = ::replxx_input(replxx_, prompt_.c_str());
  const auto line_len = strlen(line);
  if (!line) {
    return;
  }

  if (line_len == 0) {
    util::ErrorPrintf("SYNTAX ERROR: no command\n");
    return;
  }

  StringView line_view(line, line_len);
  auto       space_pos = line_view.find(' ');
  // space_pos is npos is also ok.
  cur_cmd_             = line_view.substr(0, space_pos).ToString();
  upper_cur_cmd_       = line_view.substr(0, space_pos).ToUpperString();

  if (CliCommandProcess(upper_cur_cmd_, line_view)) {
    if (is_exit) IoWait();
    return;
  }

  // * To CLI command line,
  //   we don't save them since them is redundant.
  // * To Shell command line,
  //   we save it in case user
  //   to reuse line although it is invalid.
  //
  // * To Mmkv Command  line, same with shell command.
  ::replxx_history_add(replxx_, line);
  if (::replxx_history_save(replxx_, COMMAND_HISTORY_LOCATION) < 0) {
    ::fprintf(stderr, "Failed to save the command history\n");
  }

  if (ShellCommandProcess(line_view)) {
    return;
  }

  if (ConfigCommandProcess(upper_cur_cmd_, line_view)) {
    return;
  }

  MmkvCommandProcess(upper_cur_cmd_, line_view);
}

#define _CONFIG_CHECK_TOKEN_ITER                                                                   \
  if (token_iter == tokenizer.end()) {                                                             \
    is_syntax_error = true;                                                                        \
    break;                                                                                         \
  }

#define _CONFIG_PARSE_INTEGER_TOKEN(__var)                                                         \
  {                                                                                                \
    auto token   = *token_iter;                                                                    \
    auto val_opt = kanon::StringViewToU64(token);                                                  \
    if (!val_opt) {                                                                                \
      is_syntax_error = true;                                                                      \
      break;                                                                                       \
    }                                                                                              \
    __var = *val_opt;                                                                              \
    ++token_iter;                                                                                  \
  }

#define _CONFIG_PARSE_STRING_TOKEN(__var)                                                          \
  {                                                                                                \
    auto token = *token_iter;                                                                      \
    __var      = token.ToString();                                                                 \
    ++token_iter;                                                                                  \
  }

#define _CONFIG_PARSE_STRING_VIEW_TOKEN(__var)                                                     \
  {                                                                                                \
    __var = *token_iter;                                                                           \
    ++token_iter;                                                                                  \
  }

#define _CONFIG_CHECK_TOEKN_END                                                                    \
  if (token_iter != tokenizer.end()) {                                                             \
    is_syntax_error = true;                                                                        \
    break;                                                                                         \
  }

bool MmkvClient::ConfigCommandProcess(kanon::StringView cmd, kanon::StringView line)
{
  auto conf_cmd = GetConfigCommand(cmd);
  if (conf_cmd == CONFIG_COMMAND_NUM) {
    return false;
  }

  if (!p_conf_cli_) {
    util::ErrorPrintf("ERROR: The address of configd is not specified!\n");
    return true;
  }

  line.remove_prefix(cmd.size());

  Tokenizer tokenizer(line);
  auto      token_iter      = tokenizer.begin();
  bool      is_syntax_error = false;
  switch (conf_cmd) {
    case CONFIG_FETCH_CONF: {
      p_conf_cli_->FetchConfig();
      IoWait();
    } break;

    case CONFIG_SELECT: {
      u64 node_idx = -1;
      _CONFIG_CHECK_TOKEN_ITER
      _CONFIG_PARSE_INTEGER_TOKEN(node_idx)
      _CONFIG_CHECK_TOEKN_END

      ConfigdClient::NodeEndPoint ep;
      if (p_conf_cli_->QueryNodeEndpointByNodeIdx(node_idx, &ep)) {
        SelectNode(InetAddr(ep.host, ep.port), ep.node_id);
      } else {
        util::ErrorPrintf("INDEX ERROR: The index is invalid\n");
        return true;
      };

    } break;

    case CONFIG_CACL_SHARD: {
      _CONFIG_CHECK_TOKEN_ITER
      StringView key;
      _CONFIG_PARSE_STRING_VIEW_TOKEN(key)
      _CONFIG_CHECK_TOEKN_END

      auto shard_num = p_conf_cli_ ? p_conf_cli_->ShardNum() : 0;
      auto shard_id  = MakeShardId(key);

      if (shard_num > 0) {
        shard_id %= p_conf_cli_->ShardNum();
        PrintfAndFlush("The shard id is %llu\n", (unsigned long long)shard_id);
        fflush(stdout);
      } else {
        util::ErrorPrintf(
            "ERROR: Can't calculate the shard id since there is no shard num from configd\n"
        );
      }
    } break;
    default:
      LOG_ERROR << "The handler of config command: " << GetConfigCommandString(conf_cmd)
                << " is not implemented";
  }

  if (is_syntax_error) {
    util::ErrorPrintf(
        "SYNTAX ERROR: %s%s\n",
        GetConfigCommandString(conf_cmd).c_str(),
        GetConfigCommandHint(conf_cmd).c_str()
    );
    return true;
  }
  return true;
}

void MmkvClient::Start()
{
  PrintfAndFlush("%s\n\n", APPLICATION_INFORMATION);

  if (cli_option().is_conn_configd()) {
    auto configd_addr = p_conf_cli_->cli_->GetServerAddr().ToIpPort();
    PrintfAndFlush("Connecting configd in %s...\n", configd_addr.c_str());
    p_conf_cli_->Connect();
  } else {
    auto mmkvd_addr = client_->GetServerAddr().ToIpPort();
    PrintfAndFlush("Connecting mmkvd in %s...\n", mmkvd_addr.c_str());
    client_->Connect();
  }
}

/*--------------------------------------------------*/
/* IO wait                                          */
/*--------------------------------------------------*/

void MmkvClient::IoWait() { io_latch_.IncrAndWait(); }

void MmkvClient::ConnectWait()
{
  int wait_conn_num = 0;
  if (client_) {
    wait_conn_num++;
  }
  if (p_conf_cli_) {
    wait_conn_num++;
  }
  io_latch_.ResetAndWait(wait_conn_num);
}

/*--------------------------------------------------*/
/* Connection setup                                 */
/*--------------------------------------------------*/

void MmkvClient::SetupMmkvClient(InetAddr const &server_addr)
{
  client_ = NewTcpClient(p_loop_, server_addr, "Mmkv console client");

  client_->SetConnectionCallback([this, &server_addr](TcpConnectionPtr const &conn) {
    if (conn->IsConnected()) {
      codec_.SetUpConnection(conn);
      PrintfAndFlush("Connect to %s successfully\n\n", server_addr.ToIpPort().c_str());
      // ConsoleIoProcess();
      SetPrompt();
      wait_cli_num_++;
      io_latch_.Countdown();
    } else {
      if (--wait_cli_num_ == 0) {
        if (is_exit) {
          puts("Disconnect successfully!");
          ::exit(EXIT_SUCCESS);
        } else {
          puts("\nConnection is closed by peer server");
          if (!cli_option().reconnect) ::exit(EXIT_SUCCESS);
        }
      }
    }
  });

  codec_.SetErrorCallback([](TcpConnectionPtr const &conn, MmbpCodec::ErrorCode code) {
    MMKV_UNUSED(conn);
    util::ErrorPrintf("ERROR occurred: %s", MmbpCodec::GetErrorString(code));
  });

  codec_.SetMessageCallback(
      [this](TcpConnectionPtr const &, Buffer &buffer, uint32_t, TimeStamp recv_time) {
        MmbpResponse response;
        response.ParseFrom(buffer);
        // std::cout << response->GetContent() << "\n";
        response_printer_.Printf(current_cmd_, &response);
        ::PrintfAndFlush(
            "(%.3lf sec)\n",
            (double)(recv_time.GetMicrosecondsSinceEpoch() - start_time) / 1000000
        );

        // ConsoleIoProcess();
        io_latch_.Countdown();
      }
  );
  if (cli_option().reconnect) client_->EnableRetry();
}

void MmkvClient::SetupConfigClient()
{
  if (p_conf_cli_) {
    LOG_WARN << "Config client has established";
  }
  p_conf_cli_.reset(new ConfigdClient(p_loop_, InetAddr(cli_option().configd_endpoint)));

  p_conf_cli_->codec_.SetMessageCallback([this](
                                             TcpConnectionPtr const &conn,
                                             Buffer                 &buffer,
                                             size_t                  payload_size,
                                             TimeStamp               recv_time
                                         ) {
    p_conf_cli_->OnMessage(conn, buffer, payload_size, recv_time);
    io_latch_.Countdown();
  });

  p_conf_cli_->cli_->SetConnectionCallback([this](TcpConnectionPtr const &conn) {
    p_conf_cli_->OnConnection(conn);

    if (conn->IsConnected()) {
      auto configd_addr = p_conf_cli_->cli_->GetServerAddr().ToIpPort();
      PrintfAndFlush("Connect to configd successfully\n\n");
      wait_cli_num_++;
      SetPrompt();
      io_latch_.Countdown();
    } else {
      if (--wait_cli_num_ == 0) {
        if (is_exit) {
          puts("Disconnect configd successfully!");
          ::exit(EXIT_SUCCESS);
        } else {
          puts("\nConnection is closed by configd");
          if (!cli_option().reconnect) ::exit(EXIT_SUCCESS);
        }
      }
    }
  });

  p_conf_cli_->resp_cb_ = [this](ConfigResponse const &resp) {
    switch (resp.status()) {
      case CONF_STATUS_OK:
        PrintfAndFlush("Success!\n");
        p_conf_cli_->PrintNodeConfiguration();
        break;
      case CONF_INVALID_REQ:
        PrintfAndFlush("Internal Error\n");
        break;
    }
  };

  if (cli_option().reconnect) p_conf_cli_->cli_->EnableRetry();
}

/*--------------------------------------------------*/
/* Command linenoise install(replxx)                */
/*--------------------------------------------------*/

KANON_INLINE void RegisterCommandHints(
    size_t             cmd_count,
    std::string const *cmd_strings,
    std::string const *cmd_hints,
    size_t             line_len,
    char const        *command_buf,
    size_t             cmd_len,
    std::string       &hint,
    replxx_hints      *lc
) KANON_NOEXCEPT
{
  for (size_t i = 0; i < cmd_count; ++i) {
    auto cmd_str  = cmd_strings[i];
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
static void OnCommandHint(
    char const   *line,
    replxx_hints *lc,
    int          *context_len,
    ReplxxColor  *color,
    void         *args
)
{
  *color           = ReplxxColor::REPLXX_COLOR_YELLOW;
  const size_t len = strlen(line);
  StringView   line_view(line, len);
  auto         blank_pos = line_view.find(' ');

  /* Although command isn't complete, we also should provide
     hint message */
  if (blank_pos == StringView::npos) blank_pos = len;

  auto cmd = line_view.substr(0, blank_pos).ToUpperString();

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
  *context_len     = len;
  auto cmd_strings = GetCommandStrings();
  auto cmd_hints   = GetCommandHints();
  auto cmd_count   = Command::COMMAND_NUM;

  auto cli_cmd_hints   = GetCliCommandHints();
  auto cli_cmd_count   = CliCommand::CLI_COMMAND_NUM;
  auto cli_cmd_strings = GetCliCommandStrings();

  auto config_cmd_hints   = GetConfigCommandHints();
  auto config_cmd_count   = ConfigCommand::CONFIG_COMMAND_NUM;
  auto config_cmd_strings = GetConfigCommandStrings();

  std::string hint; /* reserve space */

  RegisterCommandHints(cmd_count, cmd_strings, cmd_hints, len, cmd.data(), cmd.size(), hint, lc);
  RegisterCommandHints(
      cli_cmd_count,
      cli_cmd_strings,
      cli_cmd_hints,
      len,
      cmd.data(),
      cmd.size(),
      hint,
      lc
  );
  RegisterCommandHints(
      config_cmd_count,
      config_cmd_strings,
      config_cmd_hints,
      len,
      cmd.data(),
      cmd.size(),
      hint,
      lc
  );
}

static KANON_DEPRECATED_ATTR void OnCommandModify(char **p_line, int *cursor_pos, void *args)
{
  auto         line = *p_line;
  const size_t len  = strlen(line);
  char         command_buf[MAX_LINE];
  Command      cmd;

  for (size_t i = 0; i < len; ++i) {
    command_buf[i] = (line[i] <= 'z' && line[i] >= 'a') ? line[i] - 0x20 : line[i];
  }
  command_buf[len] = 0;
  if ((cmd = GetCommand({command_buf, len})) != Command::COMMAND_NUM) {
    auto old_line = *p_line;
    auto new_line = ::kanon::StrCat(line, GetCommandHint(cmd));
    *p_line       = (char *)calloc(new_line.size() + 1, 1);
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
static void OnCommandCompletion(
    char const         *line,
    replxx_completions *lc,
    int                *context_len,
    void               *args
)
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
    ternary_add(&command_tst, GetCliCommandString((CliCommand)i).c_str(), false);
  }

  for (size_t i = 0; i < CONFIG_COMMAND_NUM; ++i) {
    ternary_add(&command_tst, GetConfigCommandString((ConfigCommand)i).c_str(), false);
  }

  if (::replxx_history_load(replxx_, COMMAND_HISTORY_LOCATION) < 0) {
    // ::fprintf(
    //     stderr,
    //     "Failed to load the command history\n"
    //     "OR there is no command history"
    // );
  }

  ::replxx_set_indent_multiline(replxx_, prompt_.size());
  ::replxx_set_max_history_size(replxx_, 20);
  ::replxx_set_completion_callback(replxx_, &OnCommandCompletion, nullptr);
  ::replxx_set_hint_callback(replxx_, &OnCommandHint, nullptr);
  ::replxx_set_max_hint_rows(replxx_, COMMAND_NUM / 5);
  ::replxx_set_beep_on_ambiguous_completion(replxx_, true);
  //::replxx_set_modify_callback(replxx_, &OnCommandModify, nullptr);
}

void MmkvClient::SelectNode(InetAddr const &node_addr, node_id_t node_id)
{
  if (!client_ || node_id != current_peer_node_id) {
    SetupMmkvClient(node_addr);
    auto node_addr_str = node_addr.ToIpPort();
    PrintfAndFlush("Connecting mmkvd in %s\n", node_addr_str.c_str());
    client_->Connect();
    IoWait();

    SetPrompt();
    current_peer_node_id = node_id;
  }
}

void MmkvClient::SetPrompt()
{
  prompt_.clear();
  if (p_conf_cli_) {
    auto configd_addr = p_conf_cli_->cli_->GetServerAddr();
    kanon::StrAppend(prompt_, BLUE "config ", configd_addr.ToIpPort(), RESET);
  }
  if (client_) {
    auto serv_addr = client_->GetServerAddr();
    if (!prompt_.empty()) {
      prompt_ += ' ';
    }
    kanon::StrAppend(prompt_, YELLOW "mmkv ", serv_addr.ToIpPort(), RESET);
  }
  prompt_.append("> ", 2);
}