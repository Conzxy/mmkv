// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_CLIENT_OPTION_H_
#define _MMKV_CLIENT_OPTION_H_

#include <string>

namespace mmkv {
namespace client {

struct Option {
  std::string host             = "127.0.0.1";
  int         port             = 9998;
  bool        reconnect        = false;
  bool        log              = false;
  bool        version          = false;
  std::string configd_endpoint = "";

  bool is_conn_configd() const noexcept { return !configd_endpoint.empty(); }
};

Option &cli_option();

void RegisterOptions();

} // namespace client
} // namespace mmkv

#endif // _MMKV_CLIENT_OPTION_H_
