#pragma once

#include <algorithm>
#include <deque>
#include <iostream>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <string>
#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "config/config.h"
#include "disk/page_guard.h"
#include "disk/tuple.h"
#include "table/b_plus_tree_header_page.h"
#include "table/b_plus_tree_internal_page.h"
#include "table/b_plus_tree_leaf_page.h"
namespace spdb {

class Iterator {
 public:
  Iterator() = default;
  ~Iterator() = default;

  explicit Iterator(BufferPoolManager *bpm, page_id_t pid, int index,
                    std::vector<Cloum> key_type, std::vector<Cloum> value_type)
      : bpm_(bpm),
        pid_(pid),
        index_(index),
        key_type_(key_type),
        value_type_(value_type) {}

  auto IsEnd() -> bool { return pid_ == INVALID_PAGE_ID && index_ == -1; }

  auto operator*() -> const std::pair<Tuple, Tuple> & {
    auto leaf_page_guard = bpm_->FetchPageRead(pid_);
    auto leaf_page = leaf_page_guard.As<BPlusTreeLeafPage>();
    pair_ = std::make_shared<std::pair<Tuple, Tuple>>(
        leaf_page->KeyAt(index_, key_type_),
        leaf_page->ValueAt(index_, value_type_));
    return *pair_;
  }

  auto operator++() -> Iterator & {
    auto leaf_page_guard = bpm_->FetchPageRead(pid_);
    auto leaf_page = leaf_page_guard.As<BPlusTreeLeafPage>();
    ++index_;
    if (index_ >= leaf_page->GetSize()) {
      page_id_t next_id = leaf_page->GetNextPageId();
      pid_ = next_id;
      if (next_id != INVALID_PAGE_ID) {
        index_ = 0;
      } else {
        index_ = -1;
      }
    }

    return *this;
  }

  auto operator==(const Iterator &itr) const -> bool {
    return (bpm_ == itr.bpm_ && pid_ == itr.pid_ && index_ == itr.index_);
  }

  auto operator!=(const Iterator &itr) const -> bool {
    return !(bpm_ == itr.bpm_ && pid_ == itr.pid_ && index_ == itr.index_);
  }

 private:
  BufferPoolManager *bpm_;
  page_id_t pid_;
  int index_;
  std::shared_ptr<std::pair<Tuple, Tuple>> pair_;
  std::vector<Cloum> key_type_;
  std::vector<Cloum> value_type_;
};

/**
 * @brief Definition of the Context class.
 *
 * Hint: This class is designed to help you keep track of the pages
 * that you're modifying or accessing.
 */
class Context {
 public:
  Context(page_id_t pid, BufferPoolManager *bpm) {
    header_page_ = std::make_optional(bpm->FetchPageWrite(pid));
    root_page_id_ = header_page_->AsMut<BPlusTreeHeaderPage>()->root_page_id_;
  }
  // When you insert into / remove from the B+ tree, store the write guard of
  // header page here. Remember to drop the header page guard and set it to
  // nullopt when you want to unlock all.
  std::optional<WritePageGuard> header_page_{std::nullopt};

  // Save the root page id here so that it's easier to know if the current page
  // is the root page.
  page_id_t root_page_id_{INVALID_PAGE_ID};

  // Store the write guards of the pages that you're modifying here.
  std::deque<WritePageGuard> write_set_;

  // You may want to use this when getting value, but not necessary.
  std::deque<ReadPageGuard> read_set_;

  auto IsRootPage(page_id_t page_id) -> bool {
    return page_id == root_page_id_;
  }
};

class BPlusTree {
 public:
  explicit BPlusTree(std::string name, page_id_t header_page_id,
                     BufferPoolManager *buffer_pool_manager,
                     std::vector<Cloum> key_type, std::vector<Cloum> value_type,
                     int leaf_max_size, int internal_max_size);

  // Returns true if this B+ tree has no keys and values.
  auto IsEmpty() const -> bool;

  // Insert a key-value pair into this B+ tree.
  auto Insert(const Tuple &key, const Tuple &value) -> bool;

  // Remove a key and its value from this B+ tree.
  void Remove(const Tuple &key);

  // Return the value associated with a given key
  auto GetValue(const Tuple &key, Tuple &result) -> bool;

  // Return the page id of the root node
  auto GetRootPageId() -> page_id_t;

  auto Begin() -> Iterator;

  auto End() -> Iterator;

  auto Begin(const Tuple &key) -> Iterator;

 private:
  std::string name_;
  BufferPoolManager *bpm_;
  int leaf_max_size_;
  int internal_max_size_;
  page_id_t header_page_id_;
  std::vector<Cloum> key_type_;
  std::vector<Cloum> value_type_;
};

}  // namespace spdb