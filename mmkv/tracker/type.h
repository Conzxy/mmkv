// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_TRACKER_TYPE_H_
#define MMKV_TRACKER_TYPE_H_

#include <stdint.h>

#include "common_type.h"
#include "configuration.pb.h"

namespace mmkv {

static constexpr node_id_t INVALID_NODE_ID = -1;

enum ConfState {
  CONF_STATE_ADD_NODE = 0,
  CONF_STATE_LEAVE_NODE,
  // CHANGE_NODE
};

struct PendingConf {
  Configuration conf;
  ConfState     state;
  node_id_t     node_id;
};

struct PendingState {
  ConfState state;
  node_id_t node_id;
};

using NodeConfMap = ::google::protobuf::Map<uint64_t, ::mmkv::NodeConf>;

#define SHARDER_CONTROLLER_TAG      "SCPB"
#define SHARDER_CONTROLLER_MAX_SIZE (1 << 26)

} // namespace mmkv

#endif
