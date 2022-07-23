#ifndef _MMKV_SERVER_COMMON_H_
#define _MMKV_SERVER_COMMON_H_

#include <kanon/log/logger.h>

#define LOG_MMKV(_conn) \
  LOG_INFO << _conn->GetPeerAddr().ToIp()

#endif