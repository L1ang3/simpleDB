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

  // Index iterator
  // auto Begin() -> INDEXITERATOR_TYPE;

  // auto End() -> INDEXITERATOR_TYPE;

  // auto Begin(const KeyType &key) -> INDEXITERATOR_TYPE;

  // read data from file and insert one by one
  void InsertFromFile(const std::string &file_name);

  // read data from file and remove one by one
  void RemoveFromFile(const std::string &file_name);

 private:
  std::string index_name_;
  BufferPoolManager *bpm_;
  int leaf_max_size_;
  int internal_max_size_;
  page_id_t header_page_id_;
  std::vector<Cloum> key_type_;
  std::vector<Cloum> value_type_;
};

}  // namespace spdb