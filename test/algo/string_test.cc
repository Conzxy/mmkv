#include "mmkv/algo/string.h"

#include <iostream>

#include "mmkv/util/memory_footprint.h"

using namespace mmkv::algo;

int main() {
  String str = "AAA"; // SSO
  String str2;
  str2.resize(100);
  
  std::string str3 = str;  
  std::cout << str;
  mmkv::util::MemoryFootPrint();
}
