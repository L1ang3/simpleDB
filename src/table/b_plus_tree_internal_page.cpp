#include "table/b_plus_tree_internal_page.h"

namespace spdb {
void BPlusTreeInternalPage::Init(int max_size, size_t key_size,
                                 size_t value_size) {
  SetPageType(PageType::Internal);
  SetSize(0);
  SetMaxSize(max_size);
  SetKeySize(key_size);
  SetValueSize(value_size);
}

auto BPlusTreeInternalPage::KeyAt(int index, std::vector<Cloum> &key_type) const
    -> Tuple {
  size_t tuple_offset = (GetKeySize() + GetValueSize()) * index;
  auto src = (char *)malloc(GetKeySize());
  memcpy(src, data_ + tuple_offset, GetKeySize());
  Tuple ret(key_type);
  ret.SetValues(src);
  free(src);
  return ret;
}

auto BPlusTreeInternalPage::ValueAt(int index) const -> page_id_t {
  size_t tuple_offset = (GetKeySize() + GetValueSize()) * index;
  page_id_t pid = INVALID_PAGE_ID;
  memcpy(&pid, data_ + GetKeySize() + tuple_offset, GetValueSize());
  return pid;
}

void BPlusTreeInternalPage::SetKeyAt(int index, const Tuple &key) {
  size_t tuple_offset = (GetKeySize() + GetValueSize()) * index;
  memcpy(data_ + tuple_offset, key.GetData(), GetKeySize());
}

void BPlusTreeInternalPage::SetValueAt(int index, page_id_t value) {
  size_t tuple_offset = (GetKeySize() + GetValueSize()) * index;
  memcpy(data_ + tuple_offset + GetKeySize(), &value, GetValueSize());
}

auto BPlusTreeInternalPage::BinarySearch(const Tuple &key,
                                         std::vector<Cloum> &key_type) const
    -> int {
  int l = 1;
  int r = GetSize() - 1;
  if (r == 0) {
    return r;
  }
  int ret = 0;

  while (true) {
    int m = (l + r) / 2;
    if (key > KeyAt(m, key_type)) {
      if (m + 1 >= GetSize() || key < KeyAt(m + 1, key_type)) {
        ret = m;
        break;
      }
      l = m + 1;
    } else if (key < KeyAt(m, key_type)) {
      if (m - 1 <= 0 || key > KeyAt(m - 1, key_type)) {
        ret = m - 1;
        break;
      }
      r = m - 1;
    } else {
      ret = m;
      break;
    }
  }
  return ret;
}

auto BPlusTreeInternalPage::Insert(const Tuple &key, const page_id_t &value,
                                   std::vector<Cloum> &key_type) -> int {
  int ret = 1;

  if (GetSize() < GetMaxSize()) {
    for (int i = GetSize() - 1; i >= 0; i--) {
      if (i == 0) {
        SetKeyAt(i + 1, key);
        SetValueAt(i + 1, value);
        IncreaseSize(1);
        break;
      }

      if (key < KeyAt(i, key_type)) {
        SetKeyAt(i + 1, KeyAt(i, key_type));
        SetValueAt(i + 1, ValueAt(i));
      } else {
        SetKeyAt(i + 1, key);
        SetValueAt(i + 1, value);
        IncreaseSize(1);
        break;
      }
    }
    ret = 0;
  }
  return ret;
}

auto BPlusTreeInternalPage::Split(const Tuple &key, const page_id_t &value,
                                  BufferPoolManager *bpm, Tuple &key_to_insert,
                                  page_id_t &pid_to_insert,
                                  std::vector<Cloum> &key_type)
    -> BasicPageGuard {
  auto split_page_guard = bpm->NewPageGuarded(&pid_to_insert);
  auto split_page = split_page_guard.AsMut<BPlusTreeInternalPage>();
  split_page->Init(GetMaxSize(), GetKeySize(), GetValueSize());

  int lpagenums = (GetSize() + 1) / 2;
  if (key > KeyAt(lpagenums, key_type)) {
    key_to_insert = KeyAt(lpagenums, key_type);
    split_page->SetValueAt(0, ValueAt(lpagenums));
    split_page->IncreaseSize(1);
    split_page->Insert(key, value, key_type);
    for (int i = lpagenums + 1; i < GetSize(); i++) {
      split_page->Insert(KeyAt(i, key_type), ValueAt(i), key_type);
    }
    SetSize(lpagenums);
  } else if (key > KeyAt(lpagenums - 1, key_type)) {
    key_to_insert = key;
    split_page->SetValueAt(0, value);
    split_page->IncreaseSize(1);
    for (int i = lpagenums; i < GetSize(); i++) {
      split_page->Insert(KeyAt(i, key_type), ValueAt(i), key_type);
    }
    SetSize(lpagenums);
  } else {
    --lpagenums;
    key_to_insert = KeyAt(lpagenums, key_type);
    split_page->SetValueAt(0, ValueAt(lpagenums));
    split_page->IncreaseSize(1);
    for (int i = lpagenums + 1; i < GetSize(); i++) {
      split_page->Insert(KeyAt(i, key_type), ValueAt(i), key_type);
    }
    SetSize(lpagenums);
    Insert(key, value, key_type);
  }
  return split_page_guard;
}

auto BPlusTreeInternalPage::Delete(const Tuple &key, BufferPoolManager *bpm,
                                   bool have_father,
                                   std::vector<Cloum> &key_type) -> int {
  int index = BinarySearch(key, key_type);
  if (index == -1) {
    return 0;
  }

  int ret = 0;
  for (int i = index + 1; i < GetSize(); i++) {
    SetKeyAt(i - 1, KeyAt(i, key_type));
    SetValueAt(i - 1, ValueAt(i));
  }
  SetSize(GetSize() - 1);
  if (!have_father && GetSize() == 1) {
    ret = -1;
  } else if (GetSize() >= GetMinSize()) {
    ret = 0;

  } else {
    ret = 1;
  }
  return ret;
}
}  // namespace spdb