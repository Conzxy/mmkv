// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv_session.h"

#include "mmkv/algo/hash_util.h"
#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/protocol/mmbp_type.h"
#include "mmkv/util/macro.h"
#include "mmkv/util/conv.h"
#include "mmkv/storage/db.h"

#include "mmkv/disk/request_log.h"
#include "mmkv/disk/log_command.h"

#include "mmkv_server.h"
#include "common.h"
#include "config.h"

#include <kanon/util/ptr.h>

using namespace kanon;
using namespace mmkv::protocol;
using namespace mmkv::server;
using namespace mmkv::disk;
using namespace mmkv::storage;
using namespace mmkv;

MmkvSession::MmkvSession(TcpConnectionPtr const &conn, MmkvServer *server)
  : conn_(conn.get())
  , server_(server)
{
}

MmkvSession::~MmkvSession() noexcept {}

