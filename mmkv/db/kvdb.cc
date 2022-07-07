#include "kvdb.h"
#include "mmkv/db/mmkv_data.h"
#include "mmkv/db/vset.h"

using namespace mmkv::db;
using namespace mmkv::protocol;
using namespace kanon;

MmkvDb::MmkvDb() {

}

MmkvDb::~MmkvDb() noexcept {

}

bool MmkvDb::Type(String const& key, DataType& type) noexcept {
  auto kv = dict_.Find(key);
  if (!kv) return false; 

  type = kv->value.type;
  return true;
}

bool MmkvDb::Delete(String const& k) {
  auto node = dict_.Extract(k);

  if (!node) {
    return false;
  }
  
  auto& value = node->value.value;

  switch (value.type) {
    case D_STRING:
      delete (String*)value.any_data;
      break;
    case D_STRLIST:
      delete (StrList*)value.any_data;
      break;
    case D_SORTED_SET:
      delete (Vset*)value.any_data;
      break;
  }

  return true;
}

StatusCode MmkvDb::Rename(String const& old_name, String&& new_name) {
  // FIXME
  auto exists = dict_.Find(new_name);
  if (exists) return S_EXISTS;

  auto node = dict_.Extract(old_name);
  if (!node) return S_NONEXISTS;

  auto kv = dict_.InsertKv(std::move(new_name), std::move(node->value.value));
  assert(kv);
  dict_.FreeNode(node);
  return S_OK;
}

StatusCode MmkvDb::InsertStr(String k, String v) {
  MmkvData data = {
    .type = D_STRING,
    .any_data = nullptr, // dummy
  };

  auto kv = dict_.InsertKv(std::move(k), std::move(data));
  if (!kv) return S_EXISTS;
  kv->value.any_data = new String(std::move(v));

  return S_OK;
}

StatusCode MmkvDb::EraseStr(String const& k) {
  auto slot = dict_.FindSlot(k);
  auto& str = (*slot)->value.value;

  if (slot) {
    if (str.type == D_STRING) {
      delete (String*)str.any_data;
      dict_.EraseAfterFindSlot(*slot);
      return S_OK;
    } else {
      return S_EXISITS_DIFF_TYPE;
    }
  }

  return S_NONEXISTS;
}

StatusCode MmkvDb::GetStr(String const& k, String*& str) noexcept {
  KeyValue<String, MmkvData>* data = dict_.Find(k);
  if (data) {
    if (data->value.type == D_STRING) {
      str = (String*)data->value.any_data;
      return S_OK;
    }
    else return S_EXISITS_DIFF_TYPE;
  }
  return S_NONEXISTS;
}

#define LIST_ERROR_ROUTINE \
  auto kv = dict_.Find(k); \
  if (!kv) return S_NONEXISTS; \
  if (kv->value.type != D_STRLIST) return S_EXISITS_DIFF_TYPE

StatusCode MmkvDb::ListAdd(String k, StrValues& elems) {
  MmkvData data = {
    .type = D_STRLIST,
    .any_data = nullptr, // dummy
  };

  auto ret = dict_.InsertKv(std::move(k), std::move(data));
  if (!ret) return S_EXISTS;

  StrList* lst = new StrList();
  for (auto& elem : elems) { 
    lst->PushBack(std::move(elem));
  }

  ret->value.any_data = lst;

  return S_OK;
}

StatusCode MmkvDb::ListAppend(String const& k, StrValues& elems) {
  LIST_ERROR_ROUTINE;

  auto lst = (StrList*)(kv->value.any_data);
  for (auto& elem : elems) {
    lst->PushBack(std::move(elem));
  }

  return S_OK;
}


StatusCode MmkvDb::ListPrepend(String const& k, StrValues& elems) {
  LIST_ERROR_ROUTINE;

  auto lst = (StrList*)(kv->value.any_data);
  for (auto& elem : elems) {
    lst->PushFront(std::move(elem));
  }

  return S_OK;
}

StatusCode MmkvDb::ListGetSize(String const& k, size_t& size) {
  LIST_ERROR_ROUTINE;

  auto lst = (StrList*)kv->value.any_data;
  size = lst->size();
  return S_OK;
}


StatusCode MmkvDb::ListGetAll(String const& k, StrValues& values) {
  LIST_ERROR_ROUTINE;
  auto lst = (StrList*)kv->value.any_data;
  values.resize(lst->size());

  auto beg = lst->begin();
  for (size_t i = 0; i < lst->size(); ++i) {
    values[i] = *beg;
    ++beg;
  }

  return S_OK;
}


StatusCode MmkvDb::ListGetRange(String const& k, StrValues& values, size_t l, size_t r) {
  LIST_ERROR_ROUTINE;
  
  auto lst = (StrList*)kv->value.any_data;

  if (l >= lst->size()) {
    return S_INVALID_RANGE;
  }

  const auto size = DB_MIN(r, lst->size()) - l;
  values.resize(r-l);

  auto beg = lst->begin();
  while (l--) {
    ++beg;
  }
  
  for (size_t i = 0; i < size; ++i) {
    values[i] = *beg;
    ++beg;
  }

  return S_OK;
}


StatusCode MmkvDb::ListPopFront(String const& k, uint32_t count) {
  LIST_ERROR_ROUTINE;

  auto lst = (StrList*)kv->value.any_data;
  count = DB_MIN(count, lst->size());
  
  // FIXME implement PopFront(count) 
  while (count--) { 
    lst->PopFront();
  }

  return S_OK;
}


StatusCode MmkvDb::ListPopBack(String const& k, uint32_t count) {
  LIST_ERROR_ROUTINE;

  auto lst = (StrList*)kv->value.any_data;
  count = DB_MIN(count, lst->size()); 
  while (count--) {
    lst->PopBack();
  }

  return S_OK;
}


StatusCode MmkvDb::ListDel(String const& k) {
  auto slot = dict_.FindSlot(k);
  auto& str = (*slot)->value.value;

  if (slot) {
    if (str.type == D_STRLIST) {
      delete (StrList*)str.any_data;
      dict_.EraseAfterFindSlot(*slot);
      return S_OK;
    } else {
      return S_EXISITS_DIFF_TYPE;
    }
  }

  return S_NONEXISTS;
}

#define TO_VSET(kv) ((Vset*)(kv->value.any_data))

StatusCode MmkvDb::VsetAdd(String&& key, WeightValues&& wms, size_t& count) {
  // auto kv = dict_.Find(key);

  // if (!kv) {
  //   MmkvData data {
  //     .type = D_SORTED_SET,
  //     .any_data = new Vset(wms),
  //   };

  //   auto ret = dict_.InsertKv(std::move(key), std::move(data));(void)ret;
  //   count = wms.size();
  //   assert(ret);
  // } else {
  //   if (kv->value.type != D_SORTED_SET) {
  //     return S_EXISITS_DIFF_TYPE;
  //   }
  //   auto vset = (Vset*)(kv->value.any_data);
  //   count = 0;
  //   for (auto& wm : wms) {
  //     count += (int)vset->Insert(wm.key, std::move(wm.value));
  //   }
  // }

  // return S_OK;
  MmkvData data {
    .type = D_SORTED_SET,
    .any_data = nullptr, // dummy
  };

  Dict::value_type* duplicate = nullptr;
  auto success = dict_.InsertKvWithDuplicate(std::move(key), std::move(data), duplicate);

  if (success || duplicate->value.type == D_SORTED_SET) {
    Vset* vset = nullptr;
    if (success) {
      vset = new Vset();
      duplicate->value.any_data = vset;
    } else {
      vset = TO_VSET(duplicate);
    }

    count = 0;
    for (auto& wm : wms) {
      count += (int)vset->Insert(wm.key, std::move(wm.value));
    }
    return S_OK;
  }

  assert(!success && duplicate->value.type != D_SORTED_SET);
  return S_EXISITS_DIFF_TYPE;
}

#define ERROR_ROUTINE \
  auto kv = dict_.Find(key); \
  if (!kv) return S_NONEXISTS; \
  if (kv->value.type != D_SORTED_SET) return S_EXISITS_DIFF_TYPE

StatusCode MmkvDb::VsetDel(String const& key, String const& member) {
  ERROR_ROUTINE;
  if (TO_VSET(kv)->Erase(member)) {
    return S_OK;
  }
  return S_VMEMBER_NONEXISTS;
}

StatusCode MmkvDb::VsetDelRange(String const& key, OrderRange range, size_t& count) {
  ERROR_ROUTINE;
  count = TO_VSET(kv)->EraseRange(range.left, range.right);
  return S_OK;
}

StatusCode MmkvDb::VsetDelRangeByWeight(String const& key, WeightRange range, size_t& count) {
  ERROR_ROUTINE;
  count = TO_VSET(kv)->EraseRangeByWeight(range.left, range.right);
  return S_OK;
}

StatusCode MmkvDb::VsetSize(String const& key, size_t& count) {
  ERROR_ROUTINE;
  count = TO_VSET(kv)->GetSize();
  return S_OK;
}

StatusCode MmkvDb::VsetSizeByWeight(String const& key, WeightRange range, size_t& count) {
  ERROR_ROUTINE;
  count = TO_VSET(kv)->GetSizeByWeight(range.left, range.right);
  return S_OK;
}

StatusCode MmkvDb::VsetWeight(String const& key, String const& member, Weight& w) {
  ERROR_ROUTINE;
  if (TO_VSET(kv)->GetWeight(member, w)) return S_OK;
  else return S_VMEMBER_NONEXISTS;
}

StatusCode MmkvDb::VsetOrder(String const& key, String const& member, size_t& order) {
  ERROR_ROUTINE;
  if (TO_VSET(kv)->GetOrder(member, order)) return S_OK;
  else return S_VMEMBER_NONEXISTS;
}

StatusCode MmkvDb::VsetROrder(String const& key, String const& member, size_t& order) {
  ERROR_ROUTINE;
  if (TO_VSET(kv)->GetROrder(member, order)) return S_OK;
  else return S_VMEMBER_NONEXISTS;
}

StatusCode MmkvDb::VsetAll(String const& key, WeightValues& wms) {
  ERROR_ROUTINE;
  TO_VSET(kv)->GetAll(wms); 
  return S_OK;
}

StatusCode MmkvDb::VsetRange(String const& key, OrderRange range, WeightValues& wms) {
  ERROR_ROUTINE;
  TO_VSET(kv)->GetRange(range.left, range.right, wms); 
  return S_OK;
}

StatusCode MmkvDb::VsetRangeByWeight(String const& key, WeightRange range, WeightValues& wms) {
  ERROR_ROUTINE;
  TO_VSET(kv)->GetRangeByWeight(range.left, range.right, wms); 
  return S_OK;
}

StatusCode MmkvDb::VsetRRange(String const& key, OrderRange range, WeightValues& wms) {
  ERROR_ROUTINE;
  
  TO_VSET(kv)->GetRRange(range.left, range.right, wms); 
  return S_OK;
}

StatusCode MmkvDb::VsetRRangeByWeight(String const& key, WeightRange range, WeightValues& wms) {
  ERROR_ROUTINE; 
  TO_VSET(kv)->GetRRangeByWeight(range.left, range.right, wms); 
  return S_OK;
}
