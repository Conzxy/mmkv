#ifndef _MMKV_STORAGE_DB_H_
#define _MMKV_STORAGE_DB_H_

#include "mmkv/db/kvdb.h"

#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_response.h"

namespace mmkv {
namespace storage {

using db::MmkvDb;
using protocol::MmbpRequest;
using protocol::MmbpResponse;

extern MmkvDb g_db;

void DbExecute(MmbpRequest& request, MmbpResponse* response);

} // server
} // mmkv

#endif 