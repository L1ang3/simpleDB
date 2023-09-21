#pragma once

#include <string>
#include <utility>
#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "config/config.h"
#include "disk/page_guard.h"
#include "disk/tuple.h"
#include "table/b_plus_tree_page.h"

namespace spdb {

/**
 *
 * Leaf page format (keys are stored in order):
 *  ----------------------------------------------------------------------
 * | HEADER | KEY(1) + VAL(1) | KEY(2) + VAL(2) | ... | KEY(n) + VAL(n)
 *  ----------------------------------------------------------------------
 *
 *  ---------------------------------------------------------------------
 * | PageType (4) | CurrentSize (4) | MaxSize (4) | KeySize (8) | ValueSize (8)
 *  ---------------------------------------------------------------------
 *  -----------------------------------------------
 * |  NextPageId (4) |
 *  -----------------------------------------------
 */
#define LEAF_HEADER_SIZE 32
class BPlusTreeLeafPage : public BPlusTreePage {
 public:
  // Delete all constructor / destructor to ensure memory safety
  BPlusTreeLeafPage() = delete;
  BPlusTreeLeafPage(const BPlusTreeLeafPage &other) = delete;

  /**
   * After creating a new leaf page from buffer pool, must call initialize
   * method to set default values
   * @param max_size Max size of the leaf node
   */
  void Init(int max_size, size_t key_size, size_t value_size);

  // helper methods
  auto GetNextPageId() const -> page_id_t;
  void SetNextPageId(page_id_t next_page_id);
  auto KeyAt(int index, std::vector<Cloum> &key_type) const -> Tuple;
  auto ValueAt(int index, std::vector<Cloum> &value_type) const -> Tuple;
  void SetKeyAt(int index, const Tuple &key);
  void SetValueAt(int index, const Tuple &value);

  auto BinarySearch(const Tuple &key, std::vector<Cloum> &key_type) const
      -> int;

  auto Insert(const Tuple &key, const Tuple &value,
              std::vector<Cloum> &key_type, std::vector<Cloum> &value_type)
      -> int;

  auto Split(const Tuple &key, const Tuple &value, BufferPoolManager *bpm,
             Tuple &key_to_insert, page_id_t &pid_to_insert,
             std::vector<Cloum> &key_type, std::vector<Cloum> &value_type)
      -> BasicPageGuard;

  auto Delete(const Tuple &key, std::vector<Cloum> &key_type,
              std::vector<Cloum> &value_type, BufferPoolManager *bpm,
              bool have_father) -> int;

 private:
  page_id_t next_page_id_;
  // Flexible array member for page data.
  char data_[0];
};
}  // namespace spdb
