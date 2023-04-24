// SPDX-LICENSE-IDENTIFIER: Apache-2.0
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

  takina::AddSection("Connect control");
  takina::AddOption({"p", "port", "Port of mmkv server", "PORT"},
                    &cli_option().port);
  takina::AddOption({"h", "host", "Hostname of mmkv server", "HOST"},
                    &cli_option().host);
  takina::AddOption(
      {"r", "reconnect", "Reconnect to server when peer close the connection"},
      &cli_option().reconnect);

  takina::AddSection("Log control");
  takina::AddOption({"l", "log", "Enable log trace/debug/... message"},
                    &cli_option().log);

  takina::AddSection("Version information");
  takina::AddOption({"v", "version", "Show the current version of mmkv"},
                    &cli_option().version);
}

} // namespace client
} // namespace mmkv
