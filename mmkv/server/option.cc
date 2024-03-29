// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "mmkv/server/option.h"

#include <takina.h>

namespace mmkv {
namespace server {

MmkvOption &mmkv_option()
{
  static MmkvOption option;
  return option;
}

} // namespace server
} // namespace mmkv
