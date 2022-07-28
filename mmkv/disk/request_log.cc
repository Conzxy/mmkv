#include "request_log.h"

#include <unistd.h>

#include "mmkv/server/config.h"

using namespace mmkv::disk;
using namespace mmkv::server;
using namespace kanon;

RequestLog mmkv::disk::g_rlog;

RequestLog::RequestLog() 
  : empty_cond_(empty_lock_) 
  , io_thread_("RequestLogBackground")
  , file_(g_config.request_log_location) 
  , fd_(::fileno(file_.fp()))
  , latch_(1)
  , running_(false)
{
}

RequestLog::~RequestLog() noexcept {
  if (running_) Stop();
}

void RequestLog::Start() {
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

      for (auto const& blk : blks) {
        file_.Append(blk.data(), blk.len());
      }

      Flush();
    }

    Flush();
  });

  latch_.Wait();
}