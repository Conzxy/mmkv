// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef KANON_HTTP_FILE_DESCRIPTOR_H
#define KANON_HTTP_FILE_DESCRIPTOR_H

#include <stddef.h>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <cstdint>

#include "kanon/util/noncopyable.h"

namespace mmkv {
namespace disk {

struct FileException : std::runtime_error {
  explicit FileException(char const *msg)
    : std::runtime_error(msg)
  {
  }

  explicit FileException(std::string const &msg)
    : std::runtime_error(msg)
  {
  }
};

/**
 * Wrap Standard C library File API
 * into RAII style
 */
class File : kanon::noncopyable {
 public:
  enum Errno {
    E_OK = 0,
    E_EOF,
    E_ERROR,
  };

  enum OpenMode : uint8_t {
    READ  = 0x1,
    WRITE = 0x2,
    TRUNC = 0x4,
    APP   = 0x8,
    BIN   = 0x10,
  };

  File()
    : fp_(NULL)
  {
  }

  /**
   * \brief Open file in specific mode
   * \param filename The path of file
   * \param mode File open mode(SHOULD be the bit operation of OpenMode)
   *
   * \exception FileException
   */
  File(char const *filename, int mode);
  File(std::string const &filename, int mode);

  /**
   * Like the constructor but don't throw exception
   * instead return false to indicates failure
   *
   * \return
   *  true -- Success
   */
  bool Open(std::string const &filename, int mode) { return Open(filename.data(), mode); }

  bool Open(char const *filename, int mode);

  ~File() noexcept;

  /**
   * \brief Read file content in len size
   * \param buf The buffer store contents
   * \param len The length you want read
   *
   * \return
   *  -1 -- failure
   *  < len -- eof
   */
  size_t Read(void *buf, size_t len);

  /**
   * \brief Read a line terminated \r\n(Win) or \n(Linux/Unix)
   * \param need_newline Save the newline characters
   *
   * \return
   *  E_EOF -- eof
   *  E_ERROR -- error
   *  0 -- OK
   */
  Errno ReadLine(std::string &line, const bool need_newline = true);

  void Write(char const *buf, size_t len) noexcept { ::fwrite(buf, 1, len, fp_); }

  void Flush() noexcept { ::fflush(fp_); }

  bool IsValid() const noexcept { return fp_ != NULL; }

  void Rewind() noexcept { ::rewind(fp_); }

  void SeekCurrent(long offset) noexcept { Seek(offset, SEEK_CUR); }
  void SeekBegin(long offset) noexcept { Seek(offset, SEEK_SET); }
  void SeekEnd(long offset) noexcept { Seek(offset, SEEK_END); }
  long GetCurrentPosition() noexcept { return ::ftell(fp_); }

  FILE *fp() const noexcept { return fp_; }
  FILE *GetFileHandler() const noexcept { return fp_; }

  size_t        GetFileSize() const noexcept;
  static size_t GetFileSize(char const *path) noexcept;

  static const size_t kInvalidReturn = static_cast<size_t>(-1); // compatible
  static const size_t INVALID_RETURN = static_cast<size_t>(-1);

 private:
  void Seek(long offset, int whence) noexcept { ::fseek(fp_, offset, whence); }

  FILE *fp_;
};

} // namespace disk
} // namespace mmkv

#endif
