#include "mmbp_request.h"
#include "mmkv/protocol/mmbp_util.h"

using namespace mmkv::protocol;
using namespace kanon;

MmbpRequest mmkv::protocol::detail::prototype;

void MmbpRequest::SerializeTo(ChunkList& buffer) const {
  SerializeField(command_, buffer);
  SerializeField(has_bits_[0], buffer);

  if (HasKey()) {
    SerializeField(key_, buffer);
  }

  if (HasValue()) {
    SerializeField(value_, buffer);
  }

  if (HasExpireTime()) {
    SerializeField(expire_time_, buffer);
  }

  // switch (command_) {
  // case STR_SET:
  // case STR_ADD:
  //   SerializeStr(key_, buffer); 
  //   SerializeStr(value_, buffer);
  //   break;
  // case STR_GET:
  // case STR_DEL:
  //   SerializeStr(key_, buffer); 
  //   break;
  // case SET_EXPIRE:
  //   SerializeStr(key_, buffer); 
  //   buffer.Append64(expire_time_);
  //   break;
  // case MEM_STAT:
  // default:
  //   break;
  //   // do nothing
  // }
}

void MmbpRequest::ParseFrom(Buffer& buffer) {
  SetField(command_, buffer);  
  SetField(has_bits_[0], buffer); 

  // switch (command_) {
  // case STR_SET:
  // case STR_ADD:
  //   SetStrField(value_, buffer);
  // case STR_GET:
  // case STR_DEL:
  //   SetStrField(key_, buffer); 
  // case SET_EXPIRE:
  //   expire_time_ = buffer.Read64();
  // case MEM_STAT:
  // default:
  //   break;
  //   // do nothing
  // }
 
  if (HasKey()) {
    SetField(key_, buffer);
  }

  if (HasValue()) {
    SetField(value_, buffer);
  }

  if (HasExpireTime()) {
    SetField(expire_time_, buffer);
  }
}
