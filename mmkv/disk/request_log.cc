#include "request_log.h"

#include <unistd.h>

#include "mmkv/protocol/mmbp_request.h"
#include "mmkv/server/config.h"

using namespace mmkv::disk;
using namespace mmkv::server;
using namespace mmkv::protocol;
using namespace kanon;

// RequestLog *mmkv::disk::g_rlog = nullptr;

RequestLog &mmkv::disk::rlog()
{
  static RequestLog rlog;
  return rlog;
}

RequestLog::RequestLog()
  : empty_cond_(empty_lock_)
  , io_thread_("RequestLogBackground")
  , file_(mmkv_config().request_log_location)
  , fd_(::fileno(file_.fp()))
  , latch_(1)
  , running_(false)
{
}

RequestLog::~RequestLog() noexcept
{
  if (running_) Stop();
}

void RequestLog::Start()
{

  io_thread_.StartRun([this]() {
    // Wait the first log
    latch_.Countdown();
    running_ = true;

    while (running_) {
      std::vector<Block> blks;
      {
        MutexGuard g(empty_lock_);

        if (blks_.empty()) {
          empty_cond_.WaitForSeconds(1);
        }
        blks_.emplace_back();
        blks_.back().swap(cur_blk_);
        blks.swap(blks_);
      }

      for (auto const &blk : blks) {
        file_.Append(blk.data(), blk.len());
      }

      Flush();
    }

    Flush();
  });

  latch_.Wait();
}

void RequestLog::AppendDel(String key)
{
  MmbpRequest request;
  Buffer buffer;
  request.SetKey();
  request.key = std::move(key);
  request.command = DEL;
  request.SerializeTo(buffer);
  buffer.Prepend32(buffer.GetReadableSize());
  Append(buffer.GetReadBegin(), buffer.GetReadableSize());
}
