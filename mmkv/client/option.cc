#include "option.h"

#include <takina.h>

namespace mmkv {
namespace client {

Option &cli_option()
{
  static Option option;
  return option;
}

void RegisterOptions()
{
  takina::AddUsage("./mmkv_cli [OPTIONS]");
  takina::AddDescription("Command-line-interface client of mmkv");

  takina::AddOption({"l", "log", "Enable log trace/debug/... message"},
                    &cli_option().log);
  takina::AddOption({"p", "port", "Port of mmkv server", "PORT"},
                    &cli_option().port);
  takina::AddOption({"h", "host", "Hostname of mmkv server", "HOST"},
                    &cli_option().host);
  takina::AddOption(
      {"r", "reconnect", "Reconnect to server when peer close the connection"},
      &cli_option().reconnect);
  takina::AddOption({"v", "version", "Show current version of mmkv"},
                    &cli_option().version);
}

} // namespace client
} // namespace mmkv
