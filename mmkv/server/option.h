#ifndef _MMKV_SERVER_OPTION_H_
#define _MMKV_SERVER_OPTION_H_

#include <string>

#include "common.h"

namespace mmkv {
namespace server {

struct MmkvOption {
  std::string config_name = "./mmkvconf.lua";
  int port = 9998;
  std::string ip = "any";
  int sharder_port = port + BACKGROUND_PORT_DIFF;
};

MmkvOption &mmkv_option();

} // option
} // mmkv

#endif
