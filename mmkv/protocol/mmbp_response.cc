#include "mmbp_response.h"
#include "mmkv/protocol/mmbp_util.h"
#include "mmkv/protocol/status_code.h"

#include <kanon/log/logger.h>

using namespace mmkv::protocol;
using namespace kanon;

MmbpResponse MmbpResponse::prototype_;

MmbpResponse::MmbpResponse() 
  : status_code_(-1) {
  ::memset(has_bits_, 0, sizeof has_bits_);
}

MmbpResponse::~MmbpResponse() noexcept {

}

void MmbpResponse::SerializeTo(ChunkList& buffer) const {
  SerializeField(status_code_, buffer);
  SerializeField(has_bits_[0], buffer);

  if (HasValue()) {
    SerializeField(value_, buffer);
  } else if (HasValues()) {
    SerializeField(values_, buffer);
  } else if (HasKvs()) {
    SerializeField(kvs_, buffer);
  } else if (HasCount()) {
    SerializeField(count_, buffer);
  }
}

void MmbpResponse::ParseFrom(Buffer& buffer) {
  SetField(status_code_, buffer);
  SetField(has_bits_[0], buffer);

  if (HasValue()) {
    SetField(value_, buffer);
  } else if (HasValues()) {
    SetField(values_, buffer);
  } else if (HasKvs()) {
    SetField(kvs_, buffer);
  } else if (HasCount()) {
    SetField(count_, buffer);
  }
}

void MmbpResponse::DebugPrint() const noexcept {
  LOG_DEBUG << "StatusCode: " << GetStatusCode();
  LOG_DEBUG << "StatusCodeMessage: " << GetStatusMessage(GetStatusCode());

  LOG_DEBUG << "HasValue: " << HasValue();
  LOG_DEBUG << "HasValues: " << HasValues();
  LOG_DEBUG << "HasKvs: " << HasValue();
  LOG_DEBUG << "HasCount: " << HasCount();
}
