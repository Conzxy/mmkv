#include "mmbp_request.h"
#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_util.h"

using namespace mmkv::protocol;
using namespace kanon;

MmbpRequest mmkv::protocol::detail::prototype;

MmbpRequest::MmbpRequest() {
  ::memset(has_bits_, 0, sizeof(has_bits_));
}

MmbpRequest::~MmbpRequest() noexcept {

}

void MmbpRequest::SerializeTo(ChunkList& buffer) const {
  SerializeField(command_, buffer);
  SerializeField(has_bits_[0], buffer);
  
  if (HasKey()) {
    SerializeField(key_, buffer, true);
  }

  if (HasValue()) {
    SerializeField(value_, buffer);
  } else if (HasValues()) {
    SerializeField(values_, buffer);
  } else if (HasKvs()) {
    SerializeField(kvs_, buffer);
  }

  if (HasExpireTime()) {
    SerializeField(expire_time_, buffer);
  }
  
}

void MmbpRequest::ParseFrom(Buffer& buffer) {
  SetField(command_, buffer);  
  SetField(has_bits_[0], buffer); 

  if (HasKey()) {
    SetField(key_, buffer, true);
  }

  if (HasValue()) {
    SetField(value_, buffer);
  } else if (HasValues()) {
    SetField(values_, buffer);
  } else if (HasKvs()) {
    SetField(kvs_, buffer);
  }

  if (HasExpireTime()) {
    SetField(expire_time_, buffer);
  }
}
