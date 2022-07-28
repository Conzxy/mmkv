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
extern uint64_t g_recv_time;

void DbExecute(MmbpRequest& request, MmbpResponse* response);
// void DbExpireAfter(MmbpRequest &request, uint64_t ms, MmbpResponse *response);

inline void DbCheckExpirationCycle() {
  g_db.CheckExpireCycle();
}

} // server
} // mmkv

#endif 