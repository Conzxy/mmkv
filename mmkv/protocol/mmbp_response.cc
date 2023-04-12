#include "mmbp_response.h"
#include "mmkv/protocol/mmbp_util.h"
#include "mmkv/protocol/status_code.h"

#include <kanon/log/logger.h>

using namespace mmkv::protocol;
using namespace kanon;

MmbpResponse MmbpResponse::prototype_;

MmbpResponse::MmbpResponse()
  : status_code(-1)
{
  ::memset(has_bits_, 0, sizeof has_bits_);
}

MmbpResponse::~MmbpResponse() noexcept {}

void MmbpResponse::SerializeTo(Buffer &buffer) const
{
  SerializeComponent(status_code, buffer);
  SerializeComponent(has_bits_[0], buffer);

  if (HasValue()) {
    SerializeComponent(value, buffer);
  } else if (HasValues()) {
    SerializeComponent(values, buffer);
  } else if (HasKvs()) {
    SerializeComponent(kvs, buffer);
  } else if (HasCount()) {
    SerializeComponent(count, buffer);
  } else if (HasVmembers()) {
    SerializeComponent(vmembers, buffer);
  }
}

void MmbpResponse::SerializeTo(ChunkList &buffer) const
{
  SerializeComponent(status_code, buffer);
  SerializeComponent(has_bits_[0], buffer);

  if (HasValue()) {
    SerializeComponent(value, buffer);
  } else if (HasValues()) {
    SerializeComponent(values, buffer);
  } else if (HasKvs()) {
    SerializeComponent(kvs, buffer);
  } else if (HasCount()) {
    SerializeComponent(count, buffer);
  } else if (HasVmembers()) {
    SerializeComponent(vmembers, buffer);
  }
}

void MmbpResponse::ParseFrom(Buffer &buffer)
{
  ParseComponent(status_code, buffer);
  ParseComponent(has_bits_[0], buffer);

  if (HasValue()) {
    ParseComponent(value, buffer);
  } else if (HasValues()) {
    ParseComponent(values, buffer);
  } else if (HasKvs()) {
    ParseComponent(kvs, buffer);
  } else if (HasCount()) {
    ParseComponent(count, buffer);
  } else if (HasVmembers()) {
    ParseComponent(vmembers, buffer);
  }
}

void MmbpResponse::DebugPrint() const noexcept
{
  LOG_DEBUG << "StatusCode: " << status_code;
  LOG_DEBUG << "StatusCodeMessage: "
            << GetStatusMessage((StatusCode)status_code);

  LOG_DEBUG << "HasValue: " << HasValue();
  LOG_DEBUG << "HasValues: " << HasValues();
  LOG_DEBUG << "HasKvs: " << HasValue();
  LOG_DEBUG << "HasCount: " << HasCount();
  LOG_DEBUG << "HasVmember: " << HasVmembers();

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
  } else if (HasCount()) {
    LOG_DEBUG << "Count: " << count;
  } else if (HasVmembers()) {
    LOG_DEBUG << "<Weight, Member>: ";
    for (auto const &wm : vmembers)
      LOG_DEBUG << "(" << wm.key << "," << wm.value << ")";
  }
}
