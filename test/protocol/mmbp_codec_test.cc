#include "mmkv/protocol/mmbp.h"
#include "mmkv/protocol/mmbp_codec.h"
#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/protocol/mmbp_util.h"

#include <kanon/log/logger.h>
#include <kanon/util/ptr.h>
#include <xxhash.h>
#include <gtest/gtest.h>

using namespace mmkv::protocol;

TEST(mmbp_codec, xxhash) {
  ChunkList output_buffer;

  output_buffer.Append("ABCDE");
  auto state = XXH32_createState();

  auto ok = XXH32_reset(state, 0) != XXH_ERROR;
  assert(ok);

  for (auto const& chunk : output_buffer) {
    LOG_DEBUG << "chunk size = " << chunk.GetReadableSize();
    ok = XXH32_update(state, chunk.GetReadBegin(), chunk.GetReadableSize()) != XXH_ERROR;
    assert(ok);
  }
  
  uint32_t checksum1 = XXH32_digest(state);
  XXH32_freeState(state);

  Buffer input_buffer;
  input_buffer.Append(output_buffer.GetFirstChunk()->ToStringView());
  
  auto checksum2 = XXH32(input_buffer.GetReadBegin(), input_buffer.GetReadableSize(), 0);  
  
  LOG_DEBUG << "output_buffer: " << output_buffer.GetFirstChunk()->ToStringView();

  LOG_DEBUG << "input_buffer: " << input_buffer.ToStringView();

  printf("checksum1 = %u\nchecksum2 = %u\n", checksum1, checksum2);
}

TEST(mmbp_codec, parse_request) {
  MmbpCodec codec(MmbpRequest::GetPrototype());
  
  ChunkList output_buffer;

  MmbpRequest msg;
  msg.SetKey("A");
  msg.SetValue("B");
  ASSERT_TRUE(msg.HasKey());
  ASSERT_TRUE(msg.HasValue());
  ASSERT_FALSE(msg.HasExpireTime());

  codec.SerializeTo(&msg, output_buffer);
  
  LOG_DEBUG << output_buffer.begin()->ToStringView(); 
  Buffer input_buffer;
  
  auto chunk_iter = output_buffer.begin(); 
  ASSERT_EQ(chunk_iter->GetReadableSize(), 4);
  input_buffer.Append(chunk_iter->ToStringView());
  ++chunk_iter;
  ASSERT_EQ(chunk_iter->GetReadableSize(), 21);
  input_buffer.Append(chunk_iter->ToStringView());
   
  LOG_DEBUG << chunk_iter->ToStringView();

  MmbpMessage* msg1;

  ASSERT_EQ(0, codec.Parse(input_buffer, msg1));
  ASSERT_EQ(0, input_buffer.GetReadableSize());

  auto request = kanon::down_pointer_cast<MmbpRequest>(msg1); 

  ASSERT_TRUE(request->HasKey());
  ASSERT_TRUE(request->HasValue());
  ASSERT_FALSE(request->HasExpireTime());
  EXPECT_EQ(request->GetKey(), "A");
  EXPECT_EQ(request->GetValue(), "B");
}

TEST(mmbp_codec, parse_response) {
  MmbpCodec codec(MmbpResponse::GetPrototype());
  
  MmbpResponse res;
  res.SetStatusCode(S_OK);
  res.SetContent("OK");
  
  ChunkList output_buffer;
  codec.SerializeTo(&res, output_buffer);
  
  Buffer input_buffer;
  MmbpMessage* msg = nullptr;
  
  for (auto const& chunk : output_buffer) {
    input_buffer.Append(chunk.ToStringView());
  } 

  ASSERT_EQ(0, codec.Parse(input_buffer, msg));
  
  ASSERT_TRUE(msg);

  auto msg1 = kanon::down_pointer_cast<MmbpResponse>(msg);
  EXPECT_EQ(msg1->GetStatusCode(), S_OK);
  EXPECT_EQ(msg1->GetContent(), "OK");

}
