#include "mmkv/util/conc_util.h"

#include <stdio.h>

using namespace mmkv::util;

int main() {
  printf("The number of cores is %d\n", GetCoreNum());
}
