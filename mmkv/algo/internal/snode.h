// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef _MMKV_ALGO_SNODE_H_
#define _MMKV_ALGO_SNODE_H_

namespace mmkv {
namespace algo {
namespace slist {

template<typename T>
struct SNode {
  SNode* next = nullptr;
  T value;
};

} // slist

} // algo
} // mmkv

#endif // _MMKV_ALGO_SNODE_H_
