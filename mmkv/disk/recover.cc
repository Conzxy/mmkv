#include "recover.h"

#include <assert.h>

#include "mmkv/storage/db.h"
#include "mmkv/protocol/mmbp_request.h"

#include "common.h"
#include "log_command.h"

using namespace mmkv::disk;
using namespace mmkv::protocol;
using namespace mmkv::storage;

static constexpr int BUFFER_SIZE = 1 << 16;

Recover::Recover()
  : file_(REQUEST_LOG_LOCALTION, File::READ)
{
}

Recover::~Recover() noexcept {

}

void Recover::ParseFromRequest() {
  Buffer buffer;
  buffer.ReserveWriteSpace(BUFFER_SIZE);

  size_t n = 0;
  MmbpRequest request;

  while ( ( n = file_.Read(buffer.GetWriteBegin(), buffer.GetWritableSize()) ) != (size_t)-1) {
    buffer.AdvanceWrite(n);
    while (buffer.GetReadableSize() >= sizeof(uint32_t)) {
      auto size = buffer.Read32();
      if (buffer.GetReadableSize() >= size - sizeof(size)) {
        request.ParseFrom(buffer);
        DbExecute(request, nullptr);
        assert(GetCommandType((Command)request.command) == CT_WRITE);
      } else {
        break;
      }
    }

    buffer.ReserveWriteSpace(BUFFER_SIZE);
    if (n < BUFFER_SIZE) break;
  }

}