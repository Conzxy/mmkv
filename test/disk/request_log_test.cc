#include "mmkv/disk/request_log.h"

#include "mmkv/protocol/mmbp_request.h"

using namespace mmkv::disk;
using namespace mmkv::protocol;
using namespace kanon;

#define STR "0123456789abcdef"
#define N 1000

int main() {
  MmbpRequest request;
  kanon::ChunkList list;
  request.command = STR_ADD;
  request.key = STR;
  request.value = STR;
  request.SetKey();
  request.SetValue();
  request.SerializeTo(list);
  auto chunk = list.GetFirstChunk();
  auto rlen = sock::ToNetworkByteOrder32(list.GetReadableSize());

  g_rlog->Start();
  for (int i = 0; i < N; ++i) {
    LOG_DEBUG << "readable size = " << chunk->GetReadableSize();
    g_rlog->Append(&rlen, sizeof rlen);
    g_rlog->Append(chunk->GetReadBegin(), chunk->GetReadableSize());
  }

  ::sleep(10);
}
