#include "mmkv/server/mmkv_server.h"

#include "takina.h"
#include "mmkv/server/option.h"
#include "mmkv/server/config.h"
#include "chisato.h"

#include "mmkv/util/conv.h"

#include <kanon/log/async_log.h>

using namespace kanon;
using namespace mmkv;
using namespace mmkv::server;

void RegisterOptions();
bool ParseConfig(std::string& path);

int main(int argc, char* argv[]) {
  RegisterOptions();
  std::string errmsg;
  const auto success = takina::Parse(argc, argv, &errmsg);
  if (!success) {
    ::fprintf(stderr, "Failed to parse the options: %s\n", errmsg.c_str());
    return 0;
  }
  
  if (g_option.log_to_file) {
    kanon::SetupAsyncLog("mmkv-server", 1 << 21, g_option.log_dir);
  }

  LOG_INFO << "Options has parsed successfully";

  errmsg.clear(); 
  if (!ParseConfig(errmsg)) {
    ::fprintf(stderr, "Failed to parse the config file\n");
    return 0;
  }

  LOG_INFO << "Config has parsed successfully";

  EventLoop loop;
  MmkvServer server(&loop);
  
  server.Start();
  loop.StartLoop();
}

inline void RegisterOptions() {
  takina::AddUsage("./mmkv_server [OPTIONS]");
  takina::AddDescription("The server of mmkv(memory key-value)");
  takina::AddOption({"c", "config", "The filename of config(default : ./.mmkv.conf)", "CONFIG_NAME"}, &g_option.config_name);
  takina::AddOption({"f", "log-file", "Log to the file"}, &g_option.log_to_file);
  takina::AddOption({"d", "log-dir", "The directory of log(default: ./log)", "DIR"}, &g_option.log_dir);
}

inline bool ParseConfig(std::string &errmsg) {
  auto success = chisato::Parse(g_option.config_name, errmsg); 

  if (!success) return false;

  auto log_method = chisato::GetField("LogMethod");

  if (log_method == "request") {
    g_config.log_method = LM_REQUEST;
  } 

  auto exp_cycle = chisato::GetField("ExpirationCheckCycle");

  auto res = util::str2u64(exp_cycle.c_str(), g_config.expiration_check_cycle);
  if (!res) return false;

  g_config.request_log_location = chisato::GetField("RequestLogLocation");

  if (g_config.request_log_location.empty()) {
    LOG_ERROR << "The RequestLogLocation field in the config file is missing";
    return false;
  }

  return true;
}
