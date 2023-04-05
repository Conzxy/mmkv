#include "mmkv/client/information.h"
#include "mmkv/client/mmkv_client.h"
#include "mmkv/client/option.h"
#include "mmkv/version.h"
#include <kanon/init.h>
#include <takina.h>

using namespace mmkv::client;
using namespace kanon;

int main(int argc, char *argv[])
{
  std::string errmsg;
  RegisterOptions();
  if (!takina::Parse(argc, argv, &errmsg)) {
    ::fprintf(stderr, "Failed to parse option: %s\n", errmsg.c_str());
    return 0;
  }

  if (cli_option().version) {
    printf("mmkv v%s\n", MMKV_VERSION_STR);
    return 0;
  }

  InstallInformation();

  kanon::SetKanonLog(cli_option().log);
  kanon::KanonInitialize();
  EventLoopThread loop_thread;
  auto loop = loop_thread.StartRun();

  InetAddr server_addr(cli_option().host, cli_option().port);
  MmkvClient client(loop, server_addr);
  client.Start();

  client.ConnectWait();

  while (1) {
    client.ConsoleIoProcess();
  }
}
