#ifndef _CHISATO_STR_SLICE_H__
#define _CHISATO_STR_SLICE_H__

#include <stddef.h>
#include <string>
#include <string.h>
#include <assert.h>

struct StrSlice {
 public:
  StrSlice(char const *str, size_t len)
    : data_(str)
    , len_(len)
  {
  }

  StrSlice(std::string const &str)
    : data_(str.c_str())
    , len_(str.size())
  {
  }
  
  StrSlice(char const *str)
    : data_(str)
    , len_(strlen(str))
  {
  }
  
  char const *data() const noexcept { return data_; }
  size_t size() const noexcept { return len_; }
  size_t len() const noexcept { return len_; }  
  
  std::string toString() const { return std::string(data_, len_); }
  int caseCmp(StrSlice const &o) const noexcept {
    auto const res = ::strncasecmp(o.data_, data_, o.len_ < len_ ? o.len_ : len_);
    // If length is equal, result is expected
    // Otherwise, length is not equal, and the compare result indicates
    // them are equivalent, the shorter slice is smaller.
    return (len_ != o.len_ && res == 0)? ((len_ < o.len_) ? -1 : 1) : res;
  }

  char operator[](size_t idx) const noexcept {
    assert(idx < len_);
    return data_[idx];
  }

  friend inline bool operator==(StrSlice const &x, StrSlice const &y) noexcept {
    return y.len_ == x.len_ && memcmp(y.data_, x.data_, x.len_) == 0;
  }

  friend inline bool operator!=(StrSlice const &x, StrSlice const &y) noexcept {
    return !(x == y);
  }

 private:
  char const *data_;
  size_t len_;
  
};

#endif // _CHISATO_STR_SLICE_H__
