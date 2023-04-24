// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_SERVER_OPTION_H_
#define _MMKV_SERVER_OPTION_H_

#include <string>

#include "common.h"

namespace mmkv {
namespace server {

struct MmkvOption {
  std::string config_name = "./mmkvconf.lua";
  int port = 9998;
  std::string ip = "*";
  int sharder_port = port + BACKGROUND_PORT_DIFF;
  bool version = false;
};

MmkvOption &mmkv_option();

} // namespace server
} // namespace mmkv

#endif
