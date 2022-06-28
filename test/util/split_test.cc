#include "mmkv/util/split.h"

#include <stdio.h>

using namespace mmkv::util;

int main() {
  StringView view = " A B C D E F G  ";

  auto tokens = SplitStringView(view);

  for (auto const& token : tokens) {
    ::fwrite(token.data(), 1, token.size(), stdout);
    ::puts("");
  }
}
