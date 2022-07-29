#include "mmkv/db/kvdb.h"

#include "mmkv/util/time_util.h"

#include <gtest/gtest.h>

using namespace mmkv::db;
using namespace mmkv::util;
using namespace mmkv::protocol;

TEST(kvdb, expire) {
  MmkvDb db;

  EXPECT_EQ(db.InsertStr("a", "b"), S_OK);
  EXPECT_EQ(db.ExpireAfter("a", GetTimeMs(), 3), S_OK);
  ::sleep(2);
  String* value = nullptr;
  EXPECT_EQ(db.GetStr("a", value), S_NONEXISTS);
}