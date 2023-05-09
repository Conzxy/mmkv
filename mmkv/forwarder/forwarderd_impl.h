// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv/forwarder/forwarderd.h"

#include "mmkv/util/macro.h"
#include "mmkv/tracker/shard_controller_client.h"
#include "forwarder.pb.h"

using namespace kanon;
using namespace mmkv;
using namespace mmkv::server;

struct Forwarderd::Impl {
  MMKV_INLINE static void SetShardController(
      Forwarderd     *p_forwarder,
      InetAddr const &controller_addr
  )
  {

    p_forwarder->p_shard_ctl_cli_.reset(new ShardControllerClient(
        p_forwarder->server_.GetLoop(),
        controller_addr,
        InetAddr(mmkv_config().sharder_endpoint)
    ));
  }
};
