#include "mmkv/server/option.h"

#include <takina.h>

namespace mmkv {
namespace server {

MmkvOption g_option;

void RegisterOptions() {
  takina::AddUsage("./mmkv_server [OPTIONS]");
  takina::AddDescription("The server of mmkv(memory key-value)");
  takina::AddOption({"c", "config", "The filename of config(default : ./.mmkv.conf)", "CONFIG_NAME"}, &g_option.config_name);
  takina::AddOption({"f", "log-file", "Log to the file"}, &g_option.log_to_file);
  takina::AddOption({"d", "log-dir", "The directory of log(default: ./log)", "DIR"}, &g_option.log_dir);
}

bool ParseOptions(int argc, char **argv, std::string &errmsg) {
  return takina::Parse(argc, argv, &errmsg);
}

} // server
} // mmkv
