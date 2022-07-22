#include "mmkv/db/kvdb.h"

#include <gtest/gtest.h>
#include <string>

using namespace mmkv::db;

#define N 100

TEST(db_str, add) {
  MmkvDb db;

  for (int i = 0; i < N; ++i) {
    std::cout << i << "\n";
    std::string si = std::to_string(i);
    std::cout << db.InsertStr(String(si.c_str(), si.size()), String(si.c_str(), si.size())) << "\n";
  }
}
