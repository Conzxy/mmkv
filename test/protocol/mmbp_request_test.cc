#include "mmkv/protocol/mmbp_request.h"

#include <string.h>

#include <kanon/log/logger.h>

#include <gtest/gtest.h>

using namespace mmkv::protocol;

TEST(mmbp_request, serialize) {
  String key = "Conzxy";
  String value = "MMKV";

  std::unique_ptr<MmbpRequest> mmbp_message(new MmbpRequest());
  mmbp_message->SetKey(key);
  mmbp_message->SetValue(value);
  mmbp_message->SetCommand(Command::STR_ADD);
  ASSERT_TRUE(mmbp_message->HasKey());
  ASSERT_TRUE(mmbp_message->HasValue());
  ASSERT_FALSE(mmbp_message->HasExpireTime());

  ChunkList output_buffer;
  mmbp_message->SerializeTo(output_buffer);
  
  ASSERT_EQ(output_buffer.Read16(), Command::STR_ADD);
  
  uint8_t has_bits[1];

  has_bits[0] = output_buffer.Read8();
  ASSERT_TRUE(TestBit(has_bits[0], 1));
  ASSERT_TRUE(TestBit(has_bits[0], 2));

  auto const key_size = output_buffer.Read32();
  ASSERT_EQ(key_size, key.size());

  auto chunk = output_buffer.GetFirstChunk();

  ASSERT_EQ(::memcmp(chunk->ToStringView().data(), key.c_str(), key.size()), 0);

  output_buffer.AdvanceRead(key_size);
  
    
  auto const value_size = output_buffer.Read32();
  ASSERT_EQ(value_size, value.size());

  ASSERT_EQ(::memcmp(chunk->ToStringView().data(), value.c_str(), value.size()), 0);

  output_buffer.AdvanceRead(value_size);
  ASSERT_EQ(output_buffer.GetReadableSize(), 0);
}

TEST(mmbp_request, parse) {
  String key = "Conzxy";
  String value = "MMKV";

  std::unique_ptr<MmbpRequest> mmbp_message(new MmbpRequest());
  mmbp_message->SetKey(key);
  mmbp_message->SetValue(value);
  mmbp_message->SetCommand(Command::STR_ADD);

  ChunkList output_buffer;

  mmbp_message->SerializeTo(output_buffer);

  Buffer input_buffer{};

  input_buffer.Append(output_buffer.GetFirstChunk()->ToStringView());
  
  MmbpRequest request;
  request.ParseFrom(input_buffer);
  
  LOG_DEBUG << input_buffer.ToStringView(); 
  ASSERT_TRUE(request.HasKey());
  ASSERT_TRUE(request.HasValue());
  ASSERT_FALSE(request.HasExpireTime());
  ASSERT_EQ(request.GetCommnd(), STR_ADD);
  ASSERT_EQ(request.GetKey(), "Conzxy");
  ASSERT_EQ(request.GetValue(), "MMKV");
}
