#include "mmkv/server/mmkv_server.h"

#include "mmkv/server/option.h"
#include "mmkv/server/config.h"
#include "mmkv/util/conv.h"

#include <takina.h>
#include <kanon/log/async_log.h>

using namespace kanon;
using namespace mmkv;
using namespace mmkv::server;

int main(int argc, char* argv[]) {
  std::string errmsg;

  RegisterOptions();
  const auto success = ParseOptions(argc, argv, errmsg);
  if (!success) {
    ::fprintf(stderr, "Failed to parse the options: \n%s\n", errmsg.c_str());
    return 0;
  }
  takina::Teardown();
  LOG_INFO << "Options has parsed successfully";
  
  RegisterConfig(g_config);
  if (!ParseConfig(errmsg)) {
    ::fprintf(stderr, "Failed to parse the config file: \n%s\n", errmsg.c_str());
    return 0;
  }
  LOG_INFO << "Config has parsed successfully";
  PrintMmkvConfig(g_config);

  if (!g_config.diagnostic_log_dir.empty()) {
    kanon::SetupAsyncLog("mmkv-server", 1 << 21, g_config.diagnostic_log_dir);
  }

  EventLoop loop;
  MmkvServer server(&loop, InetAddr(g_option.ip, g_option.port));
  
  server.Start();
  loop.StartLoop();
}
