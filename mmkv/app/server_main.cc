#include "mmkv/server/mmkv_server.h"

#include "mmkv/server/config.h"
#include "mmkv/server/option.h"
#include "mmkv/util/conv.h"

#include <kanon/log/async_log.h>
#include <takina.h>

using namespace kanon;
using namespace mmkv;
using namespace mmkv::server;

int main(int argc, char *argv[])
{
  std::string errmsg;

  RegisterOptions();
  const auto success = ParseOptions(argc, argv, errmsg);
  if (!success) {
    ::fprintf(stderr, "Failed to parse the options: \n%s\n", errmsg.c_str());
    return 0;
  }
  takina::Teardown();
  LOG_INFO << "Options has parsed successfully";

  RegisterConfig(mmkv_config());
  if (!ParseConfig(errmsg)) {
    ::fprintf(stderr, "Failed to parse the config file: \n%s\n",
              errmsg.c_str());
    return 0;
  }
  LOG_INFO << "Config has parsed successfully";
  PrintMmkvConfig(mmkv_config());

  if (!mmkv_config().diagnostic_log_dir.empty()) {
    kanon::SetupAsyncLog("mmkv-server", 1 << 21, mmkv_config().diagnostic_log_dir);
  }

  EventLoop loop;
  std::unique_ptr<InetAddr> addr;

  if (!strcasecmp(mmkv_option().ip.c_str(), "any")) {
    addr.reset(new InetAddr(mmkv_option().port));
  } else if (!strcasecmp(mmkv_option().ip.c_str(), "localhost")) {
    addr.reset(new InetAddr(mmkv_option().port, true));
  } else {
    addr.reset(new InetAddr(mmkv_option().ip, mmkv_option().port));
  }

  MmkvServer server(&loop, *addr, InetAddr(mmkv_option().sharder_port));

  server.Start();
  loop.StartLoop();
}
