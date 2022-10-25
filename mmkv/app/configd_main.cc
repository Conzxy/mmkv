#include "mmkv/configd/server.h"
#include "mmkv/server/common.h"
#include "mmkv/server/config.h"
#include "takina.h"

using namespace mmkv::server;
using namespace mmkv::configd;

struct RouterOptions {
  int router_port = 9997;
  int tracker_port = router_port + BACKGROUND_PORT_DIFF;
};

void RegisterRouterOptions(RouterOptions &opt)
{
  takina::AddUsage("./router [OPTIONS]");
  takina::AddDescription("The router-tier of the MMKV cluster");
  takina::AddOption({"rp", "router-port", "Port number of the router", "PORT"},
                    &opt.router_port);
  takina::AddSection("Background deamon options");
  takina::AddOption(
      {"tp", "tracker-port", "Port number of the tracker", "PORT"},
      &opt.tracker_port);
}

int main(int argc, char *argv[])
{
  std::string errmsg;
  RouterOptions router_options;
  RegisterRouterOptions(router_options);
  if (!takina::Parse(argc, argv, &errmsg)) {
    LOG_ERROR << "Failed to parse the options: " << errmsg;
    exit(0);
  }

  RegisterConfig(mmkv_config());
  if (!ParseConfig(errmsg)) {
    LOG_ERROR << "Failed to parse the config file: " << errmsg;
    exit(0);
  }

  auto loop = kanon::make_unique<EventLoop>();
  ConfigServer router(loop.get(), InetAddr(router_options.router_port),
                      InetAddr(router_options.tracker_port));

  router.Listen();
  loop->StartLoop();
}
