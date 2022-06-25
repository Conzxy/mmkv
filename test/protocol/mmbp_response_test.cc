#include "mmkv/protocol/mmbp_response.h"
#include "mmkv/protocol/status_code.h"

#include <iostream>
#include <gtest/gtest.h>

using namespace mmkv::protocol;

TEST(mmbp_response, serialize) {
  MmbpResponse msg;
  msg.SetError(S_INVALID_MESSAGE); 

  ChunkList output_buffer;
  msg.SerializeTo(output_buffer);

  auto first_chunk = output_buffer.GetFirstChunk();
  
  Buffer input_buffer;

  input_buffer.Append(first_chunk->ToStringView());
  
  std::cout << input_buffer.ToStringView().ToString() << std::endl;

  MmbpResponse test_response;

  test_response.ParseFrom(input_buffer);

  ASSERT_EQ(test_response.GetStatusCode(), S_INVALID_MESSAGE);
  ASSERT_TRUE(GetStatusMessage(S_INVALID_MESSAGE) == test_response.GetContent());
}

TEST(mmbp_response, parse) {

}
