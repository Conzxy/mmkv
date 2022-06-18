#include "mmkv/zstl/type_traits.h"

using namespace zstl;

int main() {
  static_assert(negation<conjunction<bool_constant<true>, bool_constant<false>>>::value, ""); 
  static_assert(negation<conjunction<bool_constant<false>, bool_constant<true>, bool_constant<true>>>::value, "");
  static_assert(conjunction<bool_constant<true>, bool_constant<true>, bool_constant<true>>::value, "");
}
