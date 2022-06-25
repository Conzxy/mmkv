#include "mmbp_response.h"
#include <sys/stat.h>

using namespace mmkv::protocol;
using namespace kanon;

MmbpResponse MmbpResponse::prototype_;

void MmbpResponse::SerializeTo(ChunkList& buffer) const {
  SerializeField(status_code_, buffer);
  SerializeField(content_, buffer);
}

void MmbpResponse::ParseFrom(Buffer& buffer) {
  SetField(status_code_, buffer);
  SetField(content_, buffer);
}
