#ifndef _MMKV_CLIENT_OPTION_H_
#define _MMKV_CLIENT_OPTION_H_

#include <string>

namespace mmkv {
namespace client {

struct Option {
  std::string host = "127.0.0.1";
  int port = 9998;
};

extern Option g_option;

} // client  
} // mmkv

#endif // _MMKV_CLIENT_OPTION_H_