#ifndef _MMKV_SERVER_OPTION_H_
#define _MMKV_SERVER_OPTION_H_

#include <string>

namespace mmkv {
namespace server {

struct MmkvOption {
  std::string config_name = "./.mmkv.conf";
  bool log_to_file = false;
  std::string log_dir = "./log";
};

extern MmkvOption g_option;

void RegisterOptions();
bool ParseOptions(int argc, char **argv, std::string &errmsg);

} // option
} // mmkv

#endif
