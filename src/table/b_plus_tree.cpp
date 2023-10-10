#include "table/b_plus_tree.h"

namespace spdb {
auto GetTypeSize(std::vector<Cloum> &type) -> size_t {
  size_t ret = 0;
  for (auto &col : type) {
    ret += col.GetSize();
  }
  return ret;
}

BPlusTree::BPlusTree(BufferPoolManager *buffer_pool_manager,
                     std::vector<Cloum> key_type, std::vector<Cloum> value_type,
                     int leaf_max_size, int internal_max_size,
                     page_id_t root_page_id)
    : bpm_(buffer_pool_manager),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size),
      root_page_id_(root_page_id),
      key_type_(key_type),
      value_type_(value_type) {}

auto BPlusTree::IsEmpty() -> bool {
  std::lock_guard<std::mutex> l(root_latch_);
  return root_page_id_ == INVALID_PAGE_ID;
}

auto BPlusTree::Insert(const Tuple &key, const Tuple &value) -> bool {
  Context ctx;
  std::lock_guard<std::mutex> l(root_latch_);
  Tuple key_to_insert(key_type_);
  page_id_t pid_to_insert;
  if (root_page_id_ == INVALID_PAGE_ID) {
    page_id_t root_id;
    auto root_page_guard = bpm_->NewPageGuarded(&root_id);
    auto root_page = root_page_guard.AsMut<BPlusTreeLeafPage>();
    root_page->Init(leaf_max_size_, GetTypeSize(key_type_),
                    GetTypeSize(value_type_));
    root_page->Insert(key, value, key_type_, value_type_);
    root_page_id_ = root_id;
    return true;
  }

  auto tmp_page_guard = bpm_->FetchPageWrite(root_page_id_);
  auto tmp_page = tmp_page_guard.As<BPlusTreePage>();
  while (!tmp_page->IsLeafPage()) {
    auto now_page = tmp_page_guard.As<BPlusTreeInternalPage>();

    page_id_t child = now_page->ValueAt(now_page->BinarySearch(key, key_type_));
    ctx.write_set_.emplace_back(std::move(tmp_page_guard));
    tmp_page_guard = bpm_->FetchPageWrite(child);
    tmp_page = tmp_page_guard.As<BPlusTreePage>();
    if (tmp_page->GetSize() != tmp_page->GetMaxSize()) {
      ctx.write_set_.clear();
    }
  }

  auto leaf_page = tmp_page_guard.AsMut<BPlusTreeLeafPage>();
  int is_split = leaf_page->Insert(key, value, key_type_, value_type_);
  BasicPageGuard split_page_guard;
  WritePageGuard father_page_guard;
  Tuple key_insert(key_type_);
  page_id_t pid_insert;
  if (is_split == 1) {
    split_page_guard = leaf_page->Split(key, value, bpm_, key_to_insert,
                                        pid_to_insert, key_type_, value_type_);
    key_insert = key_to_insert;
    pid_insert = pid_to_insert;
    if (ctx.write_set_.empty()) {
      page_id_t root_id;
      auto new_root_page_guard = bpm_->NewPageGuarded(&root_id);
      auto new_root_page = new_root_page_guard.AsMut<BPlusTreeInternalPage>();
      new_root_page->Init(internal_max_size_, GetTypeSize(key_type_),
                          sizeof(page_id_t));
      new_root_page->SetValueAt(0, tmp_page_guard.PageId());
      new_root_page->IncreaseSize(1);
      new_root_page->Insert(key_insert, pid_insert, key_type_);
      root_page_id_ = root_id;
      return true;
    }
  } else if (is_split == -1) {
    return false;
  }

  while (is_split == 1) {
    if (ctx.write_set_.empty()) {
      page_id_t root_id;
      auto new_root_page_guard = bpm_->NewPageGuarded(&root_id);
      auto new_root_page = new_root_page_guard.AsMut<BPlusTreeInternalPage>();
      new_root_page->Init(internal_max_size_, GetTypeSize(key_type_),
                          sizeof(page_id_t));
      new_root_page->SetValueAt(0, tmp_page_guard.PageId());
      new_root_page->IncreaseSize(1);
      new_root_page->Insert(key_insert, pid_insert, key_type_);
      root_page_id_ = root_id;
      break;
    }
    father_page_guard = std::move(ctx.write_set_.back());
    auto page = father_page_guard.AsMut<BPlusTreeInternalPage>();

    is_split = page->Insert(key_insert, pid_insert, key_type_);
    if (is_split == 1) {
      split_page_guard = page->Split(key_insert, pid_insert, bpm_,
                                     key_to_insert, pid_to_insert, key_type_);
    }
    key_insert = key_to_insert;
    pid_insert = pid_to_insert;
    tmp_page_guard = std::move(father_page_guard);
    ctx.write_set_.pop_back();
  }

  return true;
}

void BPlusTree::Remove(const Tuple &key) {
  Context ctx;
  std::lock_guard<std::mutex> l(root_latch_);
  if (root_page_id_ == INVALID_PAGE_ID) {
    return;
  }

  // find the target leaf page
  auto tmp_page_guard = bpm_->FetchPageWrite(root_page_id_);
  auto tmp_page = tmp_page_guard.As<BPlusTreePage>();
  while (!tmp_page->IsLeafPage()) {
    auto now_page = tmp_page_guard.As<BPlusTreeInternalPage>();
    ctx.write_set_.emplace_back(std::move(tmp_page_guard));

    page_id_t child = now_page->ValueAt(now_page->BinarySearch(key, key_type_));

    tmp_page_guard = bpm_->FetchPageWrite(child);
    tmp_page = tmp_page_guard.As<BPlusTreePage>();
    if (tmp_page->GetSize() > tmp_page->GetMinSize()) {
      ctx.write_set_.clear();
      // if (ctx.header_page_ != std::nullopt) {
      //   ctx.header_page_->Drop();
      // }
    }
  }

  // get its father page
  auto leaf_page = tmp_page_guard.AsMut<BPlusTreeLeafPage>();
  bool have_father = (tmp_page_guard.PageId() != root_page_id_);
  WritePageGuard father_page_guard;
  if (!ctx.write_set_.empty()) {
    father_page_guard = std::move(ctx.write_set_.back());
    ctx.write_set_.pop_back();
  }

  // try to delete&borrow&merge on this leaf.
  // to find its left&right sibling
  int index_to_delete = -1;
  int is_borrow =
      leaf_page->Delete(key, key_type_, value_type_, bpm_, have_father);
  if (is_borrow == 1) {
    auto father_page = father_page_guard.AsMut<BPlusTreeInternalPage>();
    int index = father_page->BinarySearch(key, key_type_);
    page_id_t left_sibling_id =
        (index - 1 >= 0) ? father_page->ValueAt(index - 1) : INVALID_PAGE_ID;
    page_id_t right_sibling_id = (index + 1 < father_page->GetSize())
                                     ? father_page->ValueAt(index + 1)
                                     : INVALID_PAGE_ID;
    if (index == father_page->GetSize() - 1) {
      auto left_sibling_guard = bpm_->FetchPageWrite(left_sibling_id);
      auto left_sibling = left_sibling_guard.AsMut<BPlusTreeLeafPage>();
      if (left_sibling->GetSize() > left_sibling->GetMinSize()) {
        leaf_page->Insert(
            left_sibling->KeyAt(left_sibling->GetSize() - 1, key_type_),
            left_sibling->ValueAt(left_sibling->GetSize() - 1, value_type_),
            key_type_, value_type_);
        left_sibling->Delete(
            left_sibling->KeyAt(left_sibling->GetSize() - 1, key_type_),
            key_type_, value_type_, bpm_, true);
        father_page->SetKeyAt(index, leaf_page->KeyAt(0, key_type_));
        is_borrow = 0;
      } else {
        for (int i = 0; i < leaf_page->GetSize(); i++) {
          left_sibling->Insert(leaf_page->KeyAt(i, key_type_),
                               leaf_page->ValueAt(i, value_type_), key_type_,
                               value_type_);
        }
        left_sibling->SetNextPageId(leaf_page->GetNextPageId());

        index_to_delete = index;
      }
    } else {
      auto right_sibling_guard = bpm_->FetchPageWrite(right_sibling_id);
      auto right_sibling = right_sibling_guard.AsMut<BPlusTreeLeafPage>();
      if (right_sibling->GetSize() > right_sibling->GetMinSize()) {
        leaf_page->Insert(right_sibling->KeyAt(0, key_type_),
                          right_sibling->ValueAt(0, value_type_), key_type_,
                          value_type_);
        right_sibling->Delete(right_sibling->KeyAt(0, key_type_), key_type_,
                              value_type_, bpm_, 1);
        father_page->SetKeyAt(index + 1, right_sibling->KeyAt(0, key_type_));
        is_borrow = 0;
      } else {
        for (int i = 0; i < right_sibling->GetSize(); i++) {
          leaf_page->Insert(right_sibling->KeyAt(i, key_type_),
                            right_sibling->ValueAt(i, value_type_), key_type_,
                            value_type_);
        }
        leaf_page->SetNextPageId(right_sibling->GetNextPageId());
        index_to_delete = index + 1;
      }
    }

  } else if (is_borrow == -1) {
    bpm_->DeletePage(root_page_id_);
    root_page_id_ = INVALID_PAGE_ID;

    return;
  }

  while (is_borrow != 0) {
    tmp_page_guard = std::move(father_page_guard);
    have_father = (tmp_page_guard.PageId() != root_page_id_);
    if (!ctx.write_set_.empty()) {
      father_page_guard = std::move(ctx.write_set_.back());
      ctx.write_set_.pop_back();
    } else {
      auto child_page = tmp_page_guard.AsMut<BPlusTreeInternalPage>();
      is_borrow =
          child_page->Delete(child_page->KeyAt(index_to_delete, key_type_),
                             bpm_, false, key_type_);
      if (is_borrow == -1) {
        root_page_id_ = child_page->ValueAt(0);
      }
      return;
    }
    auto child_page = tmp_page_guard.AsMut<BPlusTreeInternalPage>();
    bpm_->DeletePage(child_page->ValueAt(index_to_delete));
    is_borrow =
        child_page->Delete(child_page->KeyAt(index_to_delete, key_type_), bpm_,
                           have_father, key_type_);

    if (is_borrow == 1) {
      auto father_page = father_page_guard.AsMut<BPlusTreeInternalPage>();

      int index = father_page->BinarySearch(key, key_type_);
      page_id_t left_sibling_id =
          (index - 1 >= 0) ? father_page->ValueAt(index - 1) : INVALID_PAGE_ID;
      page_id_t right_sibling_id = (index + 1 < father_page->GetSize())
                                       ? father_page->ValueAt(index + 1)
                                       : INVALID_PAGE_ID;
      if (index == father_page->GetSize() - 1) {
        auto left_sibling_guard = bpm_->FetchPageWrite(left_sibling_id);
        auto left_sibling = left_sibling_guard.AsMut<BPlusTreeInternalPage>();
        if (left_sibling->GetSize() > left_sibling->GetMinSize()) {
          child_page->Insert(father_page->KeyAt(index, key_type_),
                             child_page->ValueAt(0), key_type_);
          child_page->SetValueAt(
              0, left_sibling->ValueAt(left_sibling->GetSize() - 1));
          father_page->SetKeyAt(
              index,
              left_sibling->KeyAt(left_sibling->GetSize() - 1, key_type_));
          left_sibling->Delete(
              left_sibling->KeyAt(left_sibling->GetSize() - 1, key_type_), bpm_,
              true, key_type_);
          is_borrow = 0;
        } else {
          left_sibling->Insert(father_page->KeyAt(index, key_type_),
                               child_page->ValueAt(0), key_type_);
          for (int i = 1; i < child_page->GetSize(); i++) {
            left_sibling->Insert(child_page->KeyAt(i, key_type_),
                                 child_page->ValueAt(i), key_type_);
          }

          index_to_delete = index;
        }
      } else {
        auto right_sibling_guard = bpm_->FetchPageWrite(right_sibling_id);
        auto right_sibling = right_sibling_guard.AsMut<BPlusTreeInternalPage>();
        if (right_sibling->GetSize() > right_sibling->GetMinSize()) {
          child_page->Insert(father_page->KeyAt(index + 1, key_type_),
                             right_sibling->ValueAt(0), key_type_);
          father_page->SetKeyAt(index + 1, right_sibling->KeyAt(1, key_type_));
          right_sibling->SetValueAt(0, right_sibling->ValueAt(1));
          right_sibling->Delete(right_sibling->KeyAt(1, key_type_), bpm_, true,
                                key_type_);
          is_borrow = 0;
        } else {
          child_page->Insert(father_page->KeyAt(index + 1, key_type_),
                             right_sibling->ValueAt(0), key_type_);
          for (int i = 1; i < right_sibling->GetSize(); i++) {
            child_page->Insert(right_sibling->KeyAt(i, key_type_),
                               right_sibling->ValueAt(i), key_type_);
          }

          index_to_delete = index + 1;
        }
      }
    }
  }
}

auto BPlusTree::GetValue(const Tuple &key, Tuple &result) -> bool {
  Context ctx;
  std::lock_guard<std::mutex> l(root_latch_);
  page_id_t child_page_id = root_page_id_;
  if (child_page_id == INVALID_PAGE_ID) {
    return false;
  }

  auto page_guard = bpm_->FetchPageRead(child_page_id);
  auto page = page_guard.As<BPlusTreePage>();
  while (!page->IsLeafPage()) {
    auto child_page = page_guard.As<BPlusTreeInternalPage>();

    child_page_id =
        child_page->ValueAt(child_page->BinarySearch(key, key_type_));
    page_guard = bpm_->FetchPageRead(child_page_id);
    page = page_guard.As<BPlusTreePage>();
  }

  auto leaf_page = page_guard.As<BPlusTreeLeafPage>();
  int index = leaf_page->BinarySearch(key, key_type_);
  if (index >= 0) {
    result = leaf_page->ValueAt(index, value_type_);
  } else {
    return false;
  }

  return true;
}

auto BPlusTree::GetRootPageId() -> page_id_t {
  std::lock_guard<std::mutex> l(root_latch_);
  return root_page_id_;
}

auto BPlusTree::Begin() -> Iterator {
  std::lock_guard<std::mutex> l(root_latch_);
  Context ctx;
  if (root_page_id_ == INVALID_PAGE_ID) {
    return Iterator(bpm_, INVALID_PAGE_ID, -1, key_type_, value_type_);
  }
  auto tmp_page_guard = bpm_->FetchPageRead(root_page_id_);
  auto tmp_page = tmp_page_guard.As<BPlusTreePage>();
  while (!tmp_page->IsLeafPage()) {
    auto now_page = tmp_page_guard.As<BPlusTreeInternalPage>();

    page_id_t child = now_page->ValueAt(0);

    tmp_page_guard = bpm_->FetchPageRead(child);
    tmp_page = tmp_page_guard.As<BPlusTreePage>();
  }

  return Iterator(bpm_, tmp_page_guard.PageId(), 0, key_type_, value_type_);
}

auto BPlusTree::End() -> Iterator {
  std::lock_guard<std::mutex> l(root_latch_);
  return Iterator(bpm_, INVALID_PAGE_ID, -1, key_type_, value_type_);
}

auto BPlusTree::Begin(const Tuple &key) -> Iterator {
  std::lock_guard<std::mutex> l(root_latch_);
  Context ctx;
  if (root_page_id_ == INVALID_PAGE_ID) {
    return Iterator(bpm_, INVALID_PAGE_ID, -1, key_type_, value_type_);
  }
  auto tmp_page_guard = bpm_->FetchPageRead(root_page_id_);
  auto tmp_page = tmp_page_guard.As<BPlusTreePage>();
  while (!tmp_page->IsLeafPage()) {
    auto now_page = tmp_page_guard.As<BPlusTreeInternalPage>();
    page_id_t child = now_page->ValueAt(now_page->BinarySearch(key, key_type_));

    tmp_page_guard = bpm_->FetchPageRead(child);
    tmp_page = tmp_page_guard.As<BPlusTreePage>();
  }

  auto leaf_page_guard = bpm_->FetchPageRead(tmp_page_guard.PageId());
  auto leaf_page = leaf_page_guard.As<BPlusTreeLeafPage>();
  int index = leaf_page->BinarySearch(key, key_type_);
  return Iterator(bpm_, tmp_page_guard.PageId(), index, key_type_, value_type_);
}
}  // namespace spdb
