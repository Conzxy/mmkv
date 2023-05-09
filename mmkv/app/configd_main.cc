// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv/configd/configd.h"
#include "mmkv/configd/configd_config.h"
#include "takina.h"

using namespace mmkv::server;
using namespace kanon;

struct Options {
  std::string configd_endpoint = "*:9997";
  std::string config_path      = "./configd.conf.lua";
};

void RegisterRouterOptions(Options &opt)
{
  takina::AddDescription("The config deamon of the MMKV shard cluster");
  takina::AddOption(
      {"e", "configd-endpoint", "The endpoint address of configd"},
      &opt.configd_endpoint
  );
  takina::AddSection("Background deamon options");
  takina::AddOption({"c", "config", "The config filepath", "filename"}, &opt.config_path);
}

int main(int argc, char *argv[])
{
  std::string errmsg;
  Options     options;
  char        buf[4096];
  snprintf(buf, sizeof buf, "%s [-e|--config-endpoint] [-c|--config]", argv[0]);
  takina::AddUsage(buf);
  RegisterRouterOptions(options);
  if (!takina::Parse(argc, argv, &errmsg)) {
    LOG_ERROR << "Failed to parse the options: " << errmsg;
    exit(1);
  }

  if (!ParseConfigdConfig(options.config_path.c_str())) {
    LOG_ERROR << "Failed to parse the config file: " << errmsg;
    exit(1);
  }

  DebugPrintConfigdConfig();

  EventLoop loop;

  Configd configd(
      &loop,
      InetAddr(options.configd_endpoint),
      InetAddr(configd_config().shard_controller_endpoint)
  );

  configd.Listen();
  loop.StartLoop();
}
