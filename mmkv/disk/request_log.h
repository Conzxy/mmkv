#ifndef _MMKV_DISK_REQUEST_LOG_H_
#define _MMKV_DISK_REQUEST_LOG_H_

#include <unistd.h>

#include <kanon/log/append_file.h>
#include <kanon/net/endian_api.h>
#include <kanon/string/fixed_buffer.h>
#include <kanon/thread/condition.h>
#include <kanon/thread/count_down_latch.h>
#include <kanon/thread/mutex_lock.h>
#include <kanon/thread/thread.h>
#include <kanon/util/noncopyable.h>

#include "mmkv/algo/string.h"
#include "mmkv/server/config.h"

namespace mmkv {
namespace disk {

using algo::String;
using kanon::AppendFile;
using kanon::Condition;
using kanon::CountDownLatch;
using kanon::MutexGuard;
using kanon::MutexLock;
using kanon::Thread;

class RequestLog {
  DISABLE_EVIL_COPYABLE(RequestLog)
  static constexpr size_t BUFFER_SIZE = (1 << 16);

  using Block = kanon::detail::FixedBuffer<BUFFER_SIZE>;

 public:
  RequestLog();
  ~RequestLog() noexcept;

  void Append(void const *data, size_t len) noexcept
  {
    MutexGuard g(empty_lock_);

    if (len <= cur_blk_.avali()) {
      cur_blk_.Append((char const *)data, len);
    } else {
      const auto writable = cur_blk_.avali();
      cur_blk_.Append((char const *)data, cur_blk_.avali());
      blks_.emplace_back(std::move(cur_blk_));
      empty_cond_.Notify();
      cur_blk_.reset();
      cur_blk_.Append((char const *)data + writable, len - writable);
    }
  }

  void Append16(uint16_t i)
  {
    auto ni = kanon::sock::ToNetworkByteOrder16(i);
    Append(&ni, sizeof ni);
  }

  void Append32(uint32_t i)
  {
    auto ni = kanon::sock::ToNetworkByteOrder32(i);
    Append(&ni, sizeof ni);
  }

  void Append64(uint64_t i)
  {
    auto ni = kanon::sock::ToNetworkByteOrder64(i);
    Append(&ni, sizeof ni);
  }

  void Start();

  void Stop() noexcept
  {
    running_ = false;
    empty_cond_.Notify();
    io_thread_.Join();
  }

  /**
   * \brief Append MMBP request "Del key" to file
   * The method can't append mutiple "del key" to file efficiently
   * since can't reuse the Buffer and MmbpRequest object.
   */
  void AppendDel(String key);

 private:
  void Flush() noexcept
  {
    file_.Flush();
    ::fsync(fd_);
  }

  MutexLock empty_lock_;
  Condition empty_cond_;

  Thread io_thread_;
  AppendFile file_;
  int fd_;
  CountDownLatch latch_;

  Block cur_blk_;
  std::vector<Block> blks_;
  bool running_;
};

/* Declare pointer to avoid
 * undefined initailzation sequence
 * e.g.
 * rlog() initialize before mmkv_config() parsed,
 * the location of file is undefined
 */
// extern RequestLog *g_rlog;

RequestLog &rlog();

} // namespace disk
} // namespace mmkv

#endif
