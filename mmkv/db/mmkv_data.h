// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_ALGO_MMKV_DATA_H_
#define _MMKV_ALGO_MMKV_DATA_H_

#include <utility>

#include "data_type.h"
#include "mmkv/util/macro.h"

namespace mmkv {
namespace db {

struct MmkvData {
  DataType type;     /** Explain the any_data */
  void    *any_data; /** Store any data type */

  explicit MmkvData(DataType type_)
    : type(type_)
    // In most case, nullptr used as dummy data(If data does exists, don't fill it)
    , any_data(nullptr)
  {
  }

  MMKV_INLINE MmkvData(MmkvData &&oth) noexcept
    : type(oth.type)
    , any_data(oth.any_data)
  {
    oth.any_data = nullptr;
  }

  MMKV_INLINE MmkvData &operator=(MmkvData &&oth) noexcept
  {
    std::swap(type, oth.type);
    std::swap(any_data, oth.any_data);
    return *this;
  }

  MMKV_INLINE ~MmkvData() noexcept;
};

/* This should be the destructor of MMkvData,
 * but I want it be a POD class, and delete
 * it in determinate case.
 * Pros:
 * > Avoid move and delete automatically
 * > No need to check if any_data is nullptr
 * Cons:
 * > Need call it by hand
 * But Only few commands need, so it is acceptable.
 *
 *
 * \warning Deprecated now
 */
void DeleteMmkvData(MmkvData &data);

template <typename T>
MMKV_INLINE void DeleteSpecificMmkvData(MmkvData *p_data)
{
  delete (T *)(p_data->any_data);
  p_data->any_data = nullptr;
}

MmkvData::~MmkvData() noexcept
{
  if (any_data) DeleteMmkvData(*this);
}

} // namespace db
} // namespace mmkv

#endif // _MMKV_ALGO_MMKV_DATA_H_
