#include "mmbp_request.h"
#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_util.h"
#include <gtest/internal/gtest-port.h>

using namespace mmkv::protocol;
using namespace kanon;

MmbpRequest mmkv::protocol::detail::prototype;

MmbpRequest::MmbpRequest() {
  ::memset(has_bits_, 0, sizeof(has_bits_));
}

MmbpRequest::~MmbpRequest() noexcept {

}

void MmbpRequest::SerializeTo(ChunkList& buffer) const {
  SerializeField(command, buffer);
  SerializeField(has_bits_[0], buffer);
  
  if (HasKey()) {
    SerializeField(key, buffer, true);
  }

  if (HasValue()) {
    SerializeField(value, buffer);
  } else if (HasValues()) {
    SerializeField(values, buffer);
  } else if (HasKvs()) {
    SerializeField(kvs, buffer);
  } else if (HasCount()) {
    SerializeField(count, buffer);
  } else if (HasRange()) {
    SerializeField(range.left, buffer);
    SerializeField(range.right, buffer);
  }

  if (HasExpireTime()) {
    SerializeField(expire_time, buffer);
  }
  
}

void MmbpRequest::ParseFrom(Buffer& buffer) {
  SetField(command, buffer);  
  SetField(has_bits_[0], buffer); 

  if (HasKey()) {
    SetField(key, buffer, true);
  }

  if (HasValue()) {
    SetField(value, buffer);
  } else if (HasValues()) {
    SetField(values, buffer);
  } else if (HasKvs()) {
    SetField(kvs, buffer);
  } else if (HasCount()) {
    SetField(count, buffer);
  } else if (HasRange()) {
    SetField(range.left, buffer);
    SetField(range.right, buffer);
  }

  if (HasExpireTime()) {
    SetField(expire_time, buffer);
  }
}
