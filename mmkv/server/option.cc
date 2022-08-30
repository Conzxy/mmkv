#include "mmkv/server/option.h"

#include <takina.h>

namespace mmkv {
namespace server {

MmkvOption &mmkv_option() {
  static MmkvOption option;
  return option;
}

void RegisterOptions() {
  takina::AddUsage("./mmkv_server [OPTIONS]");
  takina::AddDescription("The server of mmkv(memory key-value)");
  takina::AddOption({"c", "config", "The filename of config(default : ./mmkv.conf)", "CONFIG_NAME"}, &mmkv_option().config_name);
  takina::AddOption({"p", "port", "Port number(default: 9998)", "PORT"}, &mmkv_option().port);
  takina::AddOption({"i", "ip", "IP address(default: any)", "IP-ADDRESS"}, &mmkv_option().ip);
  takina::AddSection("Background deamon options");
  takina::AddOption({"sp", "sharder-port", "Port number of sharder(default: 19998)"}, &mmkv_option().sharder_port);
}

bool ParseOptions(int argc, char **argv, std::string &errmsg) {
  return takina::Parse(argc, argv, &errmsg);
}

} // server
} // mmkv
