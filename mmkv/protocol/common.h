#ifndef MMKV_PROTOCOL_COMMON_H_
#define MMKV_PROTOCOL_COMMON_H_

#define FIELD_SET_HAS_DEFINE(field, index, offset) \
  void Set##field() noexcept { has_bits_[index] |= (1 << offset); } \
  bool Has##field() const noexcept { return has_bits_[index] & (1 << offset); }

#endif