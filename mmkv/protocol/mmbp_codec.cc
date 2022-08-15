#include "mmbp_codec.h"
#include "mmbp.h"
#include "mmbp_response.h"
#include "status_code.h"

#include <xxhash.h>

using namespace mmkv::protocol;

char const MmbpCodec::MMBP_TAG[] = "MMBP";
uint8_t MmbpCodec::MMBP_TAG_SIZE = sizeof(MMBP_TAG)-1;

using namespace kanon;
using namespace mmkv::protocol;

MmbpCodec::MmbpCodec(MmbpMessage* prototype)
  : prototype_(prototype)
{
  SetErrorCallback([](TcpConnectionPtr const &conn, ErrorCode error_code) {
    String error_msg = GetErrorString(error_code);
    LOG_DEBUG << "Error message: " << error_msg;
    
    MmbpResponse error_response;
    error_response.status_code = S_INVALID_MESSAGE;

    OutputBuffer buffer;
    error_response.SerializeTo(buffer);

    conn->Send(buffer);
    conn->ShutdownWrite();
  });
}

MmbpCodec::MmbpCodec(MmbpMessage *prototype, TcpConnectionPtr const &conn) 
  : MmbpCodec(prototype)
{
  SetUpConnection(conn);
}

void MmbpCodec::SetUpConnection(TcpConnectionPtr const& conn) {
  conn->SetMessageCallback([this](TcpConnectionPtr const& conn, Buffer& buffer, TimeStamp recv_time) {
    while (buffer.GetReadableSize() >= SIZE_LENGTH) {
      const auto size_header = buffer.GetReadBegin32();
      
      LOG_DEBUG << "Size header = " << size_header;
      LOG_DEBUG << "Current readable size = " << buffer.GetReadableSize(); 

      if (buffer.GetReadableSize() - SIZE_LENGTH >= size_header) {
        buffer.AdvanceRead32();
        if (buffer.GetReadableSize() < size_t(MMBP_TAG_SIZE + CHECKSUM_LENGTH) || 
            buffer.GetReadableSize() >= MAX_SIZE) {
          error_cb_(conn, E_INVALID_SIZE_HEADER);
          break;
        } else {
          if (VerifyCheckSum(buffer, size_header)) {
            if (::memcmp(buffer.GetReadBegin(), MMBP_TAG, MMBP_TAG_SIZE) != 0) {
              error_cb_(conn, E_INVALID_MESSAGE);
              break;
            }
          
            buffer.AdvanceRead(MMBP_TAG_SIZE);

            message_cb_(conn, buffer, size_header-MMBP_TAG_SIZE-CHECKSUM_LENGTH, recv_time);
            buffer.AdvanceRead32();  // checksum
          } else {
            error_cb_(conn, E_INVALID_CHECKSUM);
            break;
          }
        }
      } else break;
    }

  });
}

MmbpCodec::ErrorCode MmbpCodec::Parse(Buffer &buffer, MmbpMessage *message) {
  if (buffer.GetReadableSize() >= SIZE_LENGTH) {
    const auto size_header = buffer.GetReadBegin32();
    
    LOG_DEBUG << "Size header = " << size_header;
    LOG_DEBUG << "Current readable size = " << buffer.GetReadableSize(); 

    if (buffer.GetReadableSize() - SIZE_LENGTH >= size_header) {
      buffer.AdvanceRead32();
      if (buffer.GetReadableSize() < size_t(MMBP_TAG_SIZE + CHECKSUM_LENGTH) || 
          buffer.GetReadableSize() >= MAX_SIZE) {
        return E_INVALID_SIZE_HEADER;
      } else {

        if (VerifyCheckSum(buffer, size_header)) {
          if (::memcmp(buffer.GetReadBegin(), MMBP_TAG, MMBP_TAG_SIZE) != 0) {
            return E_INVALID_MESSAGE;
          }
        
          buffer.AdvanceRead(MMBP_TAG_SIZE);
          message = prototype_->New();

          message->ParseFrom(buffer);
          
          buffer.AdvanceRead32();  // checksum
          return E_NOERROR;
        } else {
          return E_INVALID_CHECKSUM;
        }
      }
    }
  }
  
  return E_NO_COMPLETE_MESSAGE; 
}

void MmbpCodec::Send(TcpConnection *conn, MmbpMessage const *message) {
  OutputBuffer buffer;
  SerializeTo(message, buffer); 

  conn->Send(buffer);
}

void MmbpCodec::SerializeTo(MmbpMessage const* message, OutputBuffer& buffer) {
  buffer.Append(MMBP_TAG, MMBP_TAG_SIZE);
  message->SerializeTo(buffer);
  
  auto state = XXH32_createState();
  
  auto ok = XXH32_reset(state, 0) != XXH_ERROR;(void)ok;
  assert(ok && "XXH32_reset() error");
  
  for (auto const& chunk : buffer) {
    LOG_DEBUG << "chunk readable size = " << chunk.GetReadableSize();
    ok = XXH32_update(state, chunk.GetReadBegin(), chunk.GetReadableSize()) != XXH_ERROR;
    assert(ok && "XXH32_update");
  }
  
  CheckSumType checksum = XXH32_digest(state);
  LOG_DEBUG << "checksum = " << checksum;

  XXH32_freeState(state);

  buffer.Append32(checksum);
  
  buffer.Prepend32(buffer.GetReadableSize());
}

bool MmbpCodec::VerifyCheckSum(Buffer& buffer, SizeHeaderType size_header) {
  LOG_DEBUG << "calculated range: " << size_header - CHECKSUM_LENGTH;

  const auto calculated_check_sum = XXH32(buffer.GetReadBegin(), size_header-CHECKSUM_LENGTH, 0);
  CheckSumType prepared_checksum = 0;
  ::memcpy(&prepared_checksum,  buffer.GetReadBegin()+size_header-CHECKSUM_LENGTH, CHECKSUM_LENGTH);
  prepared_checksum = sock::ToHostByteOrder32(prepared_checksum);
  
  LOG_DEBUG << "calculated check sum = " << calculated_check_sum;
  LOG_DEBUG << "prepared check sum = " << prepared_checksum;  

  return calculated_check_sum == prepared_checksum;  
}

char const* MmbpCodec::GetErrorString(ErrorCode code) noexcept {
  switch (code) {
  case E_NOERROR:
    return "INFO: OK";
  case E_INVALID_MESSAGE:
    return "FATAL: Invalid message";
  case E_INVALID_CHECKSUM:
    return "FATAL: Invalid CheckSum";
  case E_INVALID_SIZE_HEADER:
    return "FATAL: Invalid size header";
  case E_NO_COMPLETE_MESSAGE:
    return "INFO: This is not a error";
  }

  return "FATAL: Unknown error";
}
