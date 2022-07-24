#include "mmkv/util/str_util.h"

#include <iostream>

using namespace mmkv::util;

int main() {
  std::string str;
  std::string addr = "127.0.0.1:9998";
  StrCat(str, "%a %s%a", "mmkv", &addr, ">");

  if (str != "mmkv 127.0.0.1:9998>") {
    std::cout << str << std::endl;
  }
}