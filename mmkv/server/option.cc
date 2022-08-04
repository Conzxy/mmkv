#include "mmkv/server/option.h"

#include <takina.h>

namespace mmkv {
namespace server {

MmkvOption g_option;

void RegisterOptions() {
  takina::AddUsage("./mmkv_server [OPTIONS]");
  takina::AddDescription("The server of mmkv(memory key-value)");
  takina::AddOption({"c", "config", "The filename of config(default : ./mmkv.conf)", "CONFIG_NAME"}, &g_option.config_name);
  takina::AddOption({"p", "port", "Port number(default: 9998)", "PORT"}, &g_option.port);
  takina::AddOption({"i", "ip", "IP address(default: any)", "IP-ADDRESS"}, &g_option.ip);
}

bool ParseOptions(int argc, char **argv, std::string &errmsg) {
  return takina::Parse(argc, argv, &errmsg);
}

} // server
} // mmkv
