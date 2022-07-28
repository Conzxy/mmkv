#ifndef _MMKV_DISK_STAT_H_
#define _MMKV_DISK_STAT_H_

#include <sys/stat.h>

#include <kanon/util/noncopyable.h>
#include <kanon/string/string_view.h>

#include "mmkv/util/exception_macro.h"

namespace mmkv {
namespace disk {

DEFINE_EXCEPTION_FROM_OTHER(StatException, std::runtime_error);

class Stat {
  DISABLE_EVIL_COPYABLE(Stat)

public:
  Stat() {
    ::memset(&buf_, 0, sizeof(buf_));
  }

  explicit Stat(kanon::StringView pathname) {
    if (!Open(pathname)) {
      throw StatException("::stat() error");
    }
  }

  explicit Stat(int fd) {
    if (!Open(fd)) {
      throw StatException("::fstat() error");
    }
  }

  bool Open(int fd) noexcept {
    if (::fstat(fd, &buf_) < 0) {
      return false;
    }

    return true;
  }

  bool Open(kanon::StringView pathname) noexcept {
    if (::stat(pathname.data(), &buf_) < 0) {
      return false;
    }

    return true;
  }

  bool IsRegular() const noexcept { return S_ISREG(buf_.st_mode); }
  bool IsDir() const noexcept { return S_ISDIR(buf_.st_mode); }
  bool IsCharacterSpecial() const noexcept { return S_ISCHR(buf_.st_mode); }
  bool IsBlockSpecial() const noexcept { return S_ISBLK(buf_.st_mode); }
  bool IsFIFO() const noexcept { return S_ISFIFO(buf_.st_mode); }
  bool IsSymLink() const noexcept { return S_ISLNK(buf_.st_mode); }
  bool IsSocket() const noexcept { return S_ISSOCK(buf_.st_mode); }
  size_t GetFileSize() const noexcept { return buf_.st_size; }

  bool IsUserR() const noexcept { return buf_.st_mode & S_IRUSR; }
  bool IsUserW() const noexcept { return buf_.st_mode & S_IWUSR; }
  bool IsUserX() const noexcept { return buf_.st_mode & S_IXUSR; }
  bool IsGroupR() const noexcept { return buf_.st_mode & S_IRGRP; }
  bool IsGroupW() const noexcept { return buf_.st_mode & S_IWGRP; }
  bool IsGroupX() const noexcept { return buf_.st_mode & S_IXGRP; }
  bool IsOtherR() const noexcept { return buf_.st_mode & S_IROTH; }
  bool IsOtherW() const noexcept { return buf_.st_mode & S_IWOTH; }
  bool IsOtherX() const noexcept { return buf_.st_mode & S_IXOTH; }

private:
  struct stat buf_;  
};

} // disk
} // mmkv

#endif
