#include "mmbp_response.h"
#include "mmkv/protocol/mmbp_util.h"
#include "mmkv/protocol/status_code.h"

#include <kanon/log/logger.h>

using namespace mmkv::protocol;
using namespace kanon;

MmbpResponse MmbpResponse::prototype_;

MmbpResponse::MmbpResponse() 
  : status_code(-1) {
  ::memset(has_bits_, 0, sizeof has_bits_);
}

MmbpResponse::~MmbpResponse() noexcept {

}

void MmbpResponse::SerializeTo(ChunkList& buffer) const {
  SerializeField(status_code, buffer);
  SerializeField(has_bits_[0], buffer);

  if (HasValue()) {
    SerializeField(value, buffer);
  } else if (HasValues()) {
    SerializeField(values, buffer);
  } else if (HasKvs()) {
    SerializeField(kvs, buffer);
  } else if (HasCount()) {
    SerializeField(count, buffer);
  } else if (HasVmembers()) {
    SerializeField(vmembers, buffer);
  }
}

void MmbpResponse::ParseFrom(Buffer& buffer) {
  SetField(status_code, buffer);
  SetField(has_bits_[0], buffer);

  if (HasValue()) {
    SetField(value, buffer);
  } else if (HasValues()) {
    SetField(values, buffer);
  } else if (HasKvs()) {
    SetField(kvs, buffer);
  } else if (HasCount()) {
    SetField(count, buffer);
  } else if (HasVmembers()) {
    SetField(vmembers, buffer);
  }
}

void MmbpResponse::DebugPrint() const noexcept {
  LOG_DEBUG << "StatusCode: " << status_code;
  LOG_DEBUG << "StatusCodeMessage: " << GetStatusMessage((StatusCode)status_code);

  LOG_DEBUG << "HasValue: " << HasValue();
  LOG_DEBUG << "HasValues: " << HasValues();
  LOG_DEBUG << "HasKvs: " << HasValue();
  LOG_DEBUG << "HasCount: " << HasCount();
  LOG_DEBUG << "HasVmember: " << HasVmembers();
}
