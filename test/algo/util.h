#include <random>
#include <string>

#include <iostream>

static char const hexs[] = "0123456789abcdef";

inline std::string GetRandomString(int count) noexcept {
  static std::default_random_engine dre;
  static std::uniform_int_distribution<int> uid(0, sizeof(hexs)-2);
  
  std::string str;
  for (int i = 0; i < count; ++i) {
    auto index = uid(dre);
    str.push_back(hexs[index]);
  }

  return str;
}
