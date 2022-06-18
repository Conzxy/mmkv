#include "mmkv/algo/hash_util.h"

#include <stdio.h>
#include <inttypes.h>

using namespace mmkv::algo;

int main() {
  std::string str("Conzxy ABCDDEFG");

  printf("hash_val(%s) = %" PRIu64 "\n", str.c_str(), Hash<std::string>{}(str));

  str = "MMKV Conzxy";
  printf("hash_val(%s) = %" PRIu64 "\n", str.c_str(), Hash<std::string>{}(str));

}
