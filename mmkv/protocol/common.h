// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_PROTOCOL_COMMON_H_
#define MMKV_PROTOCOL_COMMON_H_

#define FIELD_SET_HAS_DEFINE(field, index, offset)                             \
  void Set##field() noexcept                                                   \
  {                                                                            \
    has_bits_[index] |= (1 << offset);                                         \
  }                                                                            \
  bool Has##field() const noexcept                                             \
  {                                                                            \
    return has_bits_[index] & (1 << offset);                                   \
  }

#define DEFINE_FIELD_SET_HAS_METHOD(type__, field__, index__, offset__)        \
  type__ field__;                                                              \
  FIELD_SET_HAS_DEFINE(field__, index__, offset__)

#define DEFINE_NEW_METHOD(type__)                                              \
  type__ *New() const override                                                 \
  {                                                                            \
    return new type__();                                                       \
  }

#endif