#include "mmkv/util/tokenizer.h"

using namespace mmkv::util;

int main() {
  Tokenizer tokenizer(" A  B   C   D E F G ");

  for (auto token : tokenizer) {
    ::fwrite(token.data(), 1, token.size(), stdout);
    ::puts("");
  }
}
