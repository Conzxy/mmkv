#include "mmkv/util/print_util.h"

using namespace mmkv::util;

int main() {
  ErrorPrintf("xxx %s\n", "sss");
  printf("ssss");
  fflush(stdout);

  char buf[128];
  ::fgets(buf, sizeof buf, stdin);
}