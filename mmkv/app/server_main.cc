// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv/server/mmkv_server.h"

#include "mmkv/server/config.h"
#include "mmkv/server/option.h"
#include "mmkv/util/conv.h"
#include "mmkv/util/str_util.h"
#include "mmkv/version.h"

#include <kanon/init.h>
#include <kanon/string/string_util.h>
#include <kanon/log/async_log.h>
#include <takina.h>

using namespace kanon;
using namespace mmkv;
using namespace mmkv::server;

int main(int argc, char *argv[])
{
  std::string errmsg;

  takina::AddUsage(kanon::StrCat(argv[0], " [OPTIONS]"));
  takina::AddDescription("The server of mmkv(memory key-value)");
  takina::AddOption(
      {"c", "config", "The filename of config(default : ./mmkv.conf)", "CONFIG_NAME"},
      &mmkv_option().config_name
  );
  takina::AddOption({"p", "port", "Port number(default: 9998)", "PORT"}, &mmkv_option().port);
  takina::AddOption({"i", "ip", "IP address(default: any)", "IP-ADDRESS"}, &mmkv_option().ip);
  takina::AddSection("Background deamon options");
  takina::AddOption(
      {"sp", "sharder-port", "Port number of sharder(default: 19998)"},
      &mmkv_option().sharder_port
  );
  takina::AddSection("Version information");
  takina::AddOption({"v", "version", "Show the current version of mmkv"}, &mmkv_option().version);
  const auto success = takina::Parse(argc, argv, &errmsg);
  if (!success) {
    ::fprintf(stderr, "Failed to parse the options: \n%s\n", errmsg.c_str());
    return 0;
  }
  takina::Teardown();
  LOG_INFO << "Options has parsed successfully";

  if (mmkv_option().version) {
    printf("mmkv v%s\n", MMKV_VERSION_STR);
    return 0;
  }
  // RegisterConfig(mmkv_config());
  // if (!ParseConfig(errmsg)) {
  //   ::fprintf(stderr, "Failed to parse the config file: \n%s\n",
  //             errmsg.c_str());
  //   return 0;
  // }
  kanon::KanonInitialize();
  if (!ParseMmkvConfig(mmkv_option().config_name, mmkv_config())) {
    ::fprintf(stderr, "Failed to parse the options\n");
    return 0;
  }
  LOG_INFO << "Config has parsed successfully";
  PrintMmkvConfig(mmkv_config());

  if (!mmkv_config().diagnostic_log_dir.empty()) {
    kanon::SetupAsyncLog("mmkv-server", 1 << 21, mmkv_config().diagnostic_log_dir);
  }

  EventLoop                 loop;
  std::unique_ptr<InetAddr> addr;

  if (!strcasecmp(mmkv_option().ip.c_str(), "*") ||
      !StrCaseCompare(mmkv_option().ip.c_str(), "any"))
  {
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
