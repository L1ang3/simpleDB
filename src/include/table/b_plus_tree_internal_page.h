#pragma once

#include <queue>
#include <string>

#include "buffer/buffer_pool_manager.h"
#include "config/config.h"
#include "disk/page_guard.h"
#include "disk/tuple.h"
#include "table/b_plus_tree_page.h"

namespace spdb {

#define INTERNAL_HEADER_SIZE 28
/**
 * Store n indexed keys and n+1 child pointers (page_id) within internal page.
 * Pointer PAGE_ID(i) points to a subtree in which all keys K satisfy:
 * K(i) <= K < K(i+1).
 * NOTE: since the number of keys does not equal to number of child pointers,
 * the first key always remains invalid. That is to say, any search/lookup
 * should ignore the first key.
 *
 * Internal page format (keys are stored in increasing order):
 *  --------------------------------------------------------------------------
 * | HEADER | KEY(1)+PAGE_ID(1) | KEY(2)+PAGE_ID(2) | ... | KEY(n)+PAGE_ID(n) |
 *  --------------------------------------------------------------------------
 */
class BPlusTreeInternalPage : public BPlusTreePage {
 public:
  // Deleted to disallow initialization
  BPlusTreeInternalPage() = delete;
  BPlusTreeInternalPage(const BPlusTreeInternalPage &other) = delete;

  /**
   * Writes the necessary header information to a newly created page, must be
   * called after the creation of a new page to make a valid
   * BPlusTreeInternalPage
   * @param max_size Maximal size of the page
   */
  void Init(int max_size, size_t key_size, size_t value_size);

  /**
   * @param index The index of the key to get. Index must be non-zero.
   * @param key_type The type of the key.
   * @return Key at index
   */
  auto KeyAt(int index, std::vector<Cloum> &key_type) const -> Tuple;

  /**
   *
   * @param index The index of the key to set. Index must be non-zero.
   * @param key The new value for key
   */
  void SetKeyAt(int index, const Tuple &key);

  void SetValueAt(int index, page_id_t value);

  /**
   *
   * @param index the index
   * @return the value at the index
   */
  auto ValueAt(int index) const -> page_id_t;

  auto BinarySearch(const Tuple &key, std::vector<Cloum> &key_type) const
      -> int;

  auto Insert(const Tuple &key, const page_id_t &value,
              std::vector<Cloum> &key_type) -> int;

  auto Split(const Tuple &key, const page_id_t &value, BufferPoolManager *bpm,
             Tuple &key_to_insert, page_id_t &pid_to_insert,
             std::vector<Cloum> &key_type) -> BasicPageGuard;

  auto Delete(const Tuple &key, BufferPoolManager *bpm, bool have_father,
              std::vector<Cloum> &key_type) -> int;

 private:
  // Flexible array member for page data.
  char data_[0];
};
}  // namespace spdb
