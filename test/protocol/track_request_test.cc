#include "mmkv/protocol/track_request.h"
#include "mmkv/protocol/track_response.h"

using namespace mmkv::protocol;

int main() {
  std::unique_ptr<TrackRequest> req(new TrackRequest());
  std::unique_ptr<MmbpMessage> res(new TrackResponse());
  ((TrackResponse*)(res.get()))->addrs.emplace_back("127.0.0.1");
}