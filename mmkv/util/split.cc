// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#include "split.h"
#include <kanon/net/user_common.h>

using namespace kanon;
using namespace mmkv::util;

std::vector<StringView> mmkv::util::SplitStringView(StringView  const& str, StringView const& sp) {
  // trim
  size_t beg = 0;
  size_t end = -1;

  std::vector<StringView> ret;

  for (;;) {
    beg = str.find_first_not_of(sp, end+1);
    if (beg == StringView::npos) {
      break;
    }

    end = str.find(sp, beg+1);
    ret.emplace_back(str.data()+beg, end-beg);
  }

  return ret;
}
