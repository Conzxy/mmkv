#ifndef _MMKV_SERVER_OPTION_H_
#define _MMKV_SERVER_OPTION_H_

#include <string>

namespace mmkv {
namespace server {

struct MmkvOption {
  std::string config_name = "./mmkv.conf";
  int port = 9998;
  std::string ip = "127.0.0.1";
};

extern MmkvOption g_option;

void RegisterOptions();
bool ParseOptions(int argc, char **argv, std::string &errmsg);

} // option
} // mmkv

#endif
