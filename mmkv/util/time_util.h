#ifndef _MMKV_UTIL_TIME_UTIL_H_
#define _MMKV_UTIL_TIME_UTIL_H_

#include "kanon/util/time.h"

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <string>

namespace mmkv {
namespace util {

#define GET_TIME_ROUTINE                                                       \
  struct kanon::timeval tv;                                                    \
  ::memset(&tv, 0, sizeof tv);                                                 \
  if (kanon::GetTimeOfDay(&tv, NULL) < 0) return -1;

/**
 * \brief Get the microseconds in unix timestamp
 * \return
 *  negative -- failure
 *  otherwise, success
 */
inline int64_t GetTimeUs() noexcept
{
  GET_TIME_ROUTINE
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

/**
 * \brief Get the milliseconds in unix timestamp
 * \return
 *  negative -- failure
 *  otherwise, success
 */
inline int64_t GetTimeMs() noexcept
{
  GET_TIME_ROUTINE
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**
 * \brief Get current seconds in floating-point format
 * \return
 *  negative -- failure
 *  otherwise, success
 */
inline double GetTimeFloatSec() noexcept
{
  GET_TIME_ROUTINE
  return tv.tv_sec + (double)(tv.tv_usec) / 1000000;
}

/**
 * \brief Get current seconds in integer format
 * \return
 *  negative -- failure
 *  otherwise, success
 */
inline int64_t GetTimeSec() noexcept { return ::time(NULL); }

} // namespace util

} // namespace mmkv

#endif // _MMKV_UTIL_TIME_UTIL_H_
