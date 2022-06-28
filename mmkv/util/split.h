#ifndef _MMKV_UTIL_SPLIT_H_
#define _MMKV_UTIL_SPLIT_H_

#include <kanon/string/string_view.h>

namespace mmkv {
namespace util {

using kanon::StringView;

std::vector<StringView> SplitStringView(StringView  const& str, StringView const& sp=" ");

} // util
} // mmkv

#endif // _MMKV_UTIL_SPLIT_H_
