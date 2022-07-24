#include "mmkv/server/mmkv_server.h"

#include "mmkv/option/takina.h"
#include "mmkv/server/option.h"

#include "mmkv/server/config.h"
#include "mmkv/config/chisato.h"

using namespace kanon;
using namespace mmkv;
using namespace mmkv::server;

MmkvOption mmkv::server::g_option;
MmkvConfig mmkv::server::g_config;

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
}

inline bool ParseConfig(std::string &errmsg) {
  auto success = chisato::Parse(g_option.config_name, errmsg); 

  if (!success) return false;

  auto log_method = chisato::GetField("LogMethod");

  if (log_method == "request") {
    g_config.log_method = LM_REQUEST;
  } 

  return true;
}