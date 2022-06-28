#include "mmkv/algo/string.h"

#include <iostream>

#include "mmkv/util/memory_footprint.h"

using namespace mmkv::algo;

int main() {
  String str = "AAA"; // SSO
  String str2;
  str2.resize(100);
  
  std::cout << str;
  mmkv::util::MemoryFootPrint();
}
