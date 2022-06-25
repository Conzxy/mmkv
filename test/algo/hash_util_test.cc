#include "mmkv/algo/hash_util.h"

#include <vector>
#include <string>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

using namespace std;
using namespace mmkv::algo;

int main() {
  std::string str("Conzxy ABCDDEFG");

  printf("hash_val(%s) = %" PRIu64 "\n", str.c_str(), Hash<std::string>{}(str));

  str = "MMKV Conzxy";
  printf("hash_val(%s) = %" PRIu64 "\n", str.c_str(), Hash<std::string>{}(str));

  vector<string> buffers{ "A", "B", "C", "D", "E" };
  XXH32_state_t* const state = XXH32_createState();
  assert(state);

  auto ok = XXH32_reset(state, 0) != XXH_ERROR;
  assert(ok);
  
  uint32_t checksum = 0;
  for (auto const& buffer : buffers) {
    // checksum += XXH32(buffer.data(), buffer.size(), 0);
    ok = XXH32_update(state, buffer.data(), buffer.size()) != XXH_ERROR;
    assert(ok);
  }
  
  checksum = XXH32_digest(state);
  
  XXH32_freeState(state);

  string buffer = "ABCDE";

  uint32_t checksum2 = XXH32(buffer.data(), buffer.size(), 0);


  printf("checksum = %u, checkksum2 = %u\n", checksum, checksum2);
}
