#include "table/b_plus_tree_leaf_page.h"

namespace spdb {

void BPlusTreeLeafPage::Init(int max_size, size_t key_size, size_t value_size) {
  SetSize(0);
  SetMaxSize(max_size);
  SetPageType(PageType::Leaf);
  next_page_id_ = INVALID_PAGE_ID;
  SetKeySize(key_size);
  SetValueSize(value_size);
};

auto BPlusTreeLeafPage::GetNextPageId() const -> page_id_t {
  return next_page_id_;
}

void BPlusTreeLeafPage::SetNextPageId(page_id_t next_page_id) {
  next_page_id_ = next_page_id;
}

auto BPlusTreeLeafPage::KeyAt(int index, std::vector<Cloum> &key_type) const
    -> Tuple {
  size_t tuple_offset = (GetKeySize() + GetValueSize()) * index;
  auto src = (char *)malloc(GetKeySize());
  memcpy(src, data_ + tuple_offset, GetKeySize());
  Tuple ret(key_type);
  ret.SetValues(src);
  free(src);
  return ret;
}

auto BPlusTreeLeafPage::ValueAt(int index, std::vector<Cloum> &value_type) const
    -> Tuple {
  size_t tuple_offset = (GetKeySize() + GetValueSize()) * index;
  auto src = (char *)malloc(GetKeySize());
  memcpy(src, data_ + GetKeySize() + tuple_offset, GetValueSize());
  Tuple ret(value_type);
  ret.SetValues(src);
  free(src);
  return ret;
}

void BPlusTreeLeafPage::SetKeyAt(int index, const Tuple &key) {
  size_t tuple_offset = (GetKeySize() + GetValueSize()) * index;
  memcpy(data_ + tuple_offset, key.GetData(), GetKeySize());
}
void BPlusTreeLeafPage::SetValueAt(int index, const Tuple &value) {
  size_t tuple_offset = (GetKeySize() + GetValueSize()) * index;
  memcpy(data_ + tuple_offset + GetKeySize(), value.GetData(), GetValueSize());
}

auto BPlusTreeLeafPage::BinarySearch(const Tuple &key,
                                     std::vector<Cloum> &key_type) const
    -> int {
  int l = 0;
  int r = GetSize() - 1;
  int ret = -1;

  while (l <= r) {
    int m = (l + r) / 2;
    if (key > KeyAt(m, key_type)) {
      l = m + 1;
    } else if (key < KeyAt(m, key_type)) {
      r = m - 1;
    } else {
      ret = m;
      break;
    }
  }
  return ret;
}

auto BPlusTreeLeafPage::Insert(const Tuple &key, const Tuple &value,
                               std::vector<Cloum> &key_type,
                               std::vector<Cloum> &value_type) -> int {
  if (BinarySearch(key, key_type) != -1) {
    return -1;
  }

  int ret = -1;
  if (GetSize() < GetMaxSize()) {
    for (int i = GetSize() - 1; i >= -1; i--) {
      if (i == -1) {
        SetKeyAt(i + 1, key);
        SetValueAt(i + 1, value);
        IncreaseSize(1);
        break;
      }

      if (key < KeyAt(i, key_type)) {
        SetKeyAt(i + 1, KeyAt(i, key_type));
        SetValueAt(i + 1, ValueAt(i, value_type));
      } else if (key > KeyAt(i, key_type)) {
        SetKeyAt(i + 1, key);
        SetValueAt(i + 1, value);
        IncreaseSize(1);
        break;
      } else {
        return -1;
      }
    }
    ret = 0;
  } else {
    ret = 1;
  }
  return ret;
}

auto BPlusTreeLeafPage::Split(const Tuple &key, const Tuple &value,
                              BufferPoolManager *bpm, Tuple &key_to_insert,
                              page_id_t &pid_to_insert,
                              std::vector<Cloum> &key_type,
                              std::vector<Cloum> &value_type)
    -> BasicPageGuard {
  auto split_page_guard = bpm->NewPageGuarded(&pid_to_insert);
  auto split_page = split_page_guard.AsMut<BPlusTreeLeafPage>();
  split_page->Init(GetMaxSize(), GetKeySize(), GetValueSize());
  split_page->SetNextPageId(next_page_id_);
  next_page_id_ = pid_to_insert;

  int lpagenums = (GetSize() + 1) / 2;
  if (key > KeyAt(lpagenums - 1, key_type)) {
    split_page->Insert(key, value, key_type, value_type);
    for (int i = lpagenums; i < GetSize(); i++) {
      auto key_at_index = KeyAt(i, key_type);
      auto value_at_index = ValueAt(i, value_type);
      //   split_page->Insert(array_[i].first, array_[i].second, cmp);
      split_page->Insert(key_at_index, value_at_index, key_type, value_type);
    }
    SetSize(lpagenums);
  } else {
    --lpagenums;
    for (int i = lpagenums; i < GetSize(); i++) {
      auto key_at_index = KeyAt(i, key_type);
      auto value_at_index = ValueAt(i, value_type);
      split_page->Insert(key_at_index, value_at_index, key_type, value_type);
    }
    SetSize(lpagenums);
    Insert(key, value, key_type, value_type);
  }

  key_to_insert = split_page->KeyAt(0, key_type);
  return split_page_guard;
}

auto BPlusTreeLeafPage::Delete(const Tuple &key, std::vector<Cloum> &key_type,
                               std::vector<Cloum> &value_type,
                               BufferPoolManager *bpm, bool have_father)
    -> int {
  int index = BinarySearch(key, key_type);
  if (index == -1) {
    return 0;
  }

  int ret = 0;
  for (int i = index + 1; i < GetSize(); i++) {
    SetKeyAt(i - 1, KeyAt(i, key_type));
    SetValueAt(i - 1, ValueAt(i, value_type));
  }
  SetSize(GetSize() - 1);
  if (!have_father && GetSize() == 0) {
    ret = -1;
  } else if (!have_father || GetSize() >= GetMinSize()) {
    ret = 0;
  } else {
    ret = 1;
  }
  return ret;
}
}  // namespace spdb