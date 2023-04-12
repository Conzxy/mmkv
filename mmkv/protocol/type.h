#ifndef _MMKV_PROTOCOL_TYPE_H_
#define _MMKV_PROTOCOL_TYPE_H_

#include <vector>
#include "mmkv/algo/key_value.h"
#include "mmkv/algo/string.h"

namespace mmkv {
namespace protocol {

using algo::KeyValue;
using algo::String;

using Weight = double;
using StrKeyValue = KeyValue<String, String>;
using WeightValue = KeyValue<Weight, String>;
// 需要预分配且capacity != size ==> 采用std::vector
using StrKvs = std::vector<StrKeyValue>;
using StrValues = std::vector<String>;
using WeightValues = std::vector<KeyValue<Weight, String>>;

namespace detail {

template <typename T>
struct RangeT {
  T left;
  T right;
};

} // namespace detail

// using Range = detail::RangeT<uint32_t>;
using Range = detail::RangeT<int64_t>;
using DRange = detail::RangeT<double>;
using WeightRange = DRange;
using OrderRange = Range;

using Shard = uint32_t;

} // namespace protocol
} // namespace mmkv

#endif // _MMKV_PROTOCOL_TYPE_H_
