#ifndef _MMKV_DISK_RECOVER_H_
#define _MMKV_DISK_RECOVER_H_

#include <kanon/util/noncopyable.h>

#include "file.h"

namespace mmkv {
namespace disk {

class Recover : kanon::noncopyable {
 public:
  Recover();
  ~Recover() noexcept;

  void ParseFromRequest();
 private:
  File file_;
};

} // disk
} // mmkv

#endif // 
