#include "mmbp_request.h"
#include "mmkv/protocol/command.h"
#include "mmkv/protocol/mmbp_util.h"

#include <stdint.h>
#include <kanon/log/logger.h>

using namespace mmkv::protocol;
using namespace kanon;

MmbpRequest mmkv::protocol::detail::prototype;

MmbpRequest::MmbpRequest()
{
  ::memset(has_bits_, 0, sizeof(has_bits_));
}

MmbpRequest::~MmbpRequest() noexcept {}

void MmbpRequest::SerializeTo(Buffer &buffer) const
{
  SerializeComponent(command, buffer);
  SerializeComponent(has_bits_[0], buffer);

  if (HasKey()) {
    SerializeComponent(key, buffer, true);
  }

  if (HasValue()) {
    SerializeComponent(value, buffer);
  } else if (HasValues()) {
    SerializeComponent(values, buffer);
  } else if (HasKvs()) {
    SerializeComponent(kvs, buffer);
  } else if (HasCount()) {
    SerializeComponent(count, buffer);
  } else if (HasRange()) {
    SerializeComponent((uint64_t)range.left, buffer);
    SerializeComponent((uint64_t)range.right, buffer);
  } else if (HasVmembers()) {
    SerializeComponent(vmembers, buffer);
  }

  if (HasExpireTime()) {
    SerializeComponent(expire_time, buffer);
  }
}

void MmbpRequest::SerializeTo(ChunkList &buffer) const
{
  SerializeComponent(command, buffer);
  SerializeComponent(has_bits_[0], buffer);

  if (HasKey()) {
    SerializeComponent(key, buffer, true);
  }

  if (HasValue()) {
    SerializeComponent(value, buffer);
  } else if (HasValues()) {
    SerializeComponent(values, buffer);
  } else if (HasKvs()) {
    SerializeComponent(kvs, buffer);
  } else if (HasCount()) {
    SerializeComponent(count, buffer);
  } else if (HasRange()) {
    SerializeComponent((uint64_t)range.left, buffer);
    SerializeComponent((uint64_t)range.right, buffer);
  } else if (HasVmembers()) {
    SerializeComponent(vmembers, buffer);
  }

  if (HasExpireTime()) {
    SerializeComponent(expire_time, buffer);
  }
}

void MmbpRequest::ParseFrom(Buffer &buffer)
{
  if (buffer.GetReadableSize() >= sizeof(command))
    ParseComponent(command, buffer);
  else
    return;

  if (buffer.GetReadableSize() >= sizeof(has_bits_))
    ParseComponent(has_bits_[0], buffer);
  else
    return;

  if (HasKey()) {
    ParseComponent(key, buffer, true);
  }

  if (HasValue()) {
    ParseComponent(value, buffer);
  } else if (HasValues()) {
    ParseComponent(values, buffer);
  } else if (HasKvs()) {
    ParseComponent(kvs, buffer);
  } else if (HasCount()) {
    ParseComponent(count, buffer);
  } else if (HasRange()) {
    ParseComponent(range.left, buffer);
    ParseComponent(range.right, buffer);
  } else if (HasVmembers()) {
    ParseComponent(vmembers, buffer);
  }

  if (HasExpireTime()) {
    ParseComponent(expire_time, buffer);
  }
}

void MmbpRequest::DebugPrint() const noexcept
{
  if (command <= COMMAND_NUM) {
    LOG_DEBUG << "Command: " << GetCommandString((Command)command);
  } else
    return;

  if (HasKey()) {
    LOG_DEBUG << "Key: " << key;
  }

  if (HasValue()) {
    LOG_DEBUG << "Value: " << value;
  } else if (HasValues()) {
    LOG_DEBUG << "Value: ";
    for (auto const &value : values)
      LOG_DEBUG << value;
  } else if (HasKvs()) {
    LOG_DEBUG << "KeyValues: ";
    for (auto const &kv : kvs)
      LOG_DEBUG << "<" << kv.key << ", " << kv.value << ">";
  } else if (HasRange()) {
    LOG_DEBUG << "Range: [" << range.left << "," << range.right << ")";
    LOG_DEBUG << "DRange: [" << util::int2double(range.left) << ", "
              << util::int2double(range.right) << "]";
  } else if (HasCount()) {
    LOG_DEBUG << "Count: " << count;
  } else if (HasVmembers()) {
    LOG_DEBUG << "<Weight, Member>: ";
    for (auto const &wm : vmembers)
      LOG_DEBUG << "(" << wm.key << "," << wm.value << ")";
  }

  if (HasExpireTime()) {
    LOG_DEBUG << "ExpireTime: " << expire_time;
  }
}

void MmbpRequest::Reset()
{
  memset(has_bits_, 0, sizeof has_bits_);
  // Don't to call shrink_to_fit()
  // to reuse the old memory space
  key.clear();
  kvs.clear();
  values.clear();
  value.clear();
  vmembers.clear();
}