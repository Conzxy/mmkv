#ifndef _MMKV_SERVER_OPTION_H_
#define _MMKV_SERVER_OPTION_H_

#include <string>

namespace mmkv {
namespace server {

struct MmkvOption {
  std::string config_name = "./.mmkv.conf";
};

extern MmkvOption g_option;

} // option
} // mmkv

#endif