#include "mmkv/cluster/cluster.h"

#include <assert.h>
#include <stdio.h>

#include "mmkv/server/config.h"

using namespace mmkv::server;
using namespace mmkv;

#define ExpectEq(x, y, msg)                                                    \
  do {                                                                         \
    if ((x) != (y)) {                                                          \
      printf("error: %s\n", (msg));                                            \
    }                                                                          \
  } while (0)

#define AssertEq(x, y, msg) assert((x) == (y) && (msg))

#define NODE_NUM 10

int main()
{
  uint16_t port = 9997;
  EventLoopThread loop_thr("Router");
  Cluster cluster(loop_thr.StartRun(), InetAddr(port++));

  cluster.Start();
  
  for (size_t i = 1; i <= NODE_NUM; ++i) {
    LOG_INFO << "*** Start " << i << "th sharder ***";
    cluster.StartShardServer(InetAddr(port++));
  
    // Wait for connecting
    sleep(2);

    mmkv::Configuration config;
    cluster.Query(config);

    ExpectEq(config.size(), i, "Node size isn't expected");
    PrintConfiguartion(config);
  }

  fflush(stdout);
  sleep(60 * 10);
}
