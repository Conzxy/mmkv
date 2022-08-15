#include "mmkv/util/tokenizer.h"

using namespace mmkv::util;

void test_tokenizer(Tokenizer::Text text, char deli = ' ') {
  Tokenizer tokenizer(text, deli);
  for (auto token : tokenizer) {
    ::fwrite(token.data(), 1, token.size(), stdout);
    ::puts("");
  }
}
int main() {
  test_tokenizer(" A  B   C   D E F G ");
  test_tokenizer(" A, B,  C,  D,E,F,G x", ',');
}
