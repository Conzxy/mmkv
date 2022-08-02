#include "option.h"

#include <takina.h>

namespace mmkv {
namespace client {

Option g_option;

void RegisterOptions() {
  takina::AddUsage("./mmkv_cli [OPTIONS]");
  takina::AddDescription("Command-line-interface client of mmkv");
  takina::AddOption({"p", "port", "Port of mmkv server", "PORT"}, &g_option.port);
  takina::AddOption({"h", "host", "Hostname of mmkv server", "HOST"}, &g_option.host);
  takina::AddOption({"r", "reconnect", "Reconnect to server when peer close the connection"}, &g_option.reconnect);
}

} // client
} // mmkv
