#include "kvdb.h"
#include "mmkv/db/mmkv_data.h"

using namespace mmkv::db;
using namespace mmkv::protocol;

MmkvDb::MmkvDb() {

}

MmkvDb::~MmkvDb() noexcept {

}

int MmkvDb::InsertStr(String k, String v) {
  MmkvData data = {
    .type = D_STRING,
    .any_data = new String(std::move(v)),
  };

  return dict_.InsertKv(std::move(k), std::move(data)) ? 1 : 0;
}

int MmkvDb::EraseStr(String const& k) {
  auto node = dict_.Extract(k);

  if (node) {
    delete (String*)node->value.value.any_data;
    return 1;
  }

  return 0;
}

String* MmkvDb::GetStr(String const& k) noexcept {
  KeyValue<String, MmkvData>* data = dict_.Find(k);
  if (data && data->value.type == D_STRING) {
    return (String*)data->value.any_data;
  }
  return nullptr;
}

#define LIST_ERROR_ROUTINE(return_var) \
  if (!kv || kv->value.type != D_STRLIST) return (return_var)

bool MmkvDb::ListAdd(String k, StrValues& elems) {
  auto kv = dict_.Find(k);
  if (kv) return false;

  StrList lst{};
  for (auto& elem : elems) { 
    lst.PushBack(std::move(elem));
  }

  MmkvData data = {
    .type = D_STRLIST,
    .any_data = new StrList(std::move(lst)),
  };

  auto ret = dict_.InsertKv(std::move(k), std::move(data));
  assert(ret);
  return ret;
}

bool MmkvDb::ListAppend(String const& k, StrValues& elems) {
  auto kv = dict_.Find(k);
  LIST_ERROR_ROUTINE(false);

  auto lst = (StrList*)(kv->value.any_data);
  for (auto& elem : elems) {
    lst->PushBack(std::move(elem));
  }

  return true;
}


bool MmkvDb::ListPrepend(String const& k, StrValues& elems) {
  auto kv = dict_.Find(k);
  LIST_ERROR_ROUTINE(false);

  auto lst = (StrList*)(kv->value.any_data);
  for (auto& elem : elems) {
    lst->PushFront(std::move(elem));
  }

  return true;
}

size_t MmkvDb::ListGetSize(String const& k) {
  auto kv = dict_.Find(k);
  LIST_ERROR_ROUTINE((size_t)-1);

  auto lst = (StrList*)kv->value.any_data;
  return lst->size();
}


bool MmkvDb::ListGetAll(String const& k, StrValues& values) {
  auto kv = dict_.Find(k);
  LIST_ERROR_ROUTINE(false);

  auto lst = (StrList*)kv->value.any_data;

  values.resize(lst->size());

  auto beg = lst->begin();
  for (size_t i = 0; i < lst->size(); ++i) {
    values[i] = *beg;
    ++beg;
  }

  return true;
}


StatusCode MmkvDb::ListGetRange(String const& k, StrValues& values, size_t l, size_t r) {
  auto kv = dict_.Find(k);
  LIST_ERROR_ROUTINE(S_NONEXISTS);
  
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


bool MmkvDb::ListPopFront(String const& k, uint32_t count) {
  auto kv = dict_.Find(k);
  LIST_ERROR_ROUTINE(false);

  auto lst = (StrList*)kv->value.any_data;
  if (lst->size() < count) {
    return false;
  }
  
  // FIXME implement PopFront(count) 
  while (count--) { 
    lst->PopFront();
  }

  return true;
}


bool MmkvDb::ListPopBack(String const& k, uint32_t count) {
  auto kv = dict_.Find(k);
  LIST_ERROR_ROUTINE(false);

  auto lst = (StrList*)kv->value.any_data;
  if (lst->size() < count) {
    return false;
  }
  
  while (count--) {
    lst->PopBack();
  }

  return true;
}


bool MmkvDb::ListDel(String const& k) {
  auto node = dict_.Extract(k);
  
  if (node) {
    delete (StrList*)node->value.value.any_data;
    return true;
  }

  return false;
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
  }

  return true
}
