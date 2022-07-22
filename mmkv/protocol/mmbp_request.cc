#include "mmbp_request.h"
#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_util.h"

#include <stdint.h>
#include <kanon/log/logger.h>

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
    SerializeField((uint64_t)range.left, buffer);
    SerializeField((uint64_t)range.right, buffer);
  } else if (HasVmembers()) {
    SerializeField(vmembers, buffer);
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
  } else if (HasVmembers()) {
    SetField(vmembers, buffer);
  }

  if (HasExpireTime()) {
    SetField(expire_time, buffer);
  }
}

void MmbpRequest::DebugPrint() const noexcept {
  LOG_DEBUG << "Command: " << command_strings[command];
  if (HasKey()) {
    LOG_DEBUG << "Key: " << key;
  }

  if (HasValue()) {
    LOG_DEBUG << "Value: " << value;
  } else if (HasValues()) {
    LOG_DEBUG << "Value: ";
    for (auto const& value : values)
      LOG_DEBUG << value;
  } else if (HasKvs()) {
    LOG_DEBUG << "KeyValues: ";
    for (auto const& kv : kvs)
      LOG_DEBUG << "<" << kv.key << ", " << kv.value << ">";
  } else if (HasRange()) {
    LOG_DEBUG << "Range: [" << range.left << "," << range.right << ")";
    LOG_DEBUG << "DRange: [" << util::int2double(range.left) << ", " << util::int2double(range.right) << "]";
  } else if (HasCount()) {
    LOG_DEBUG << "Count: " << count;
  } else if (HasVmembers()) {
    LOG_DEBUG << "<Weight, Member>: ";
    for (auto const& wm : vmembers)
      LOG_DEBUG << "(" << wm.key << "," << wm.value << ")";
  }

  if (HasExpireTime()) {
    LOG_DEBUG << "ExpireTime: " << expire_time;
  }
}