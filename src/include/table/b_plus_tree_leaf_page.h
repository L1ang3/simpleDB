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

#define LEAF_PAGE_HEADER_SIZE 16

/**
 * Store indexed key and record id(record id = page id combined with slot id,
 * see include/common/rid.h for detailed implementation) together within leaf
 * page. Only support unique key.
 *
 * Leaf page format (keys are stored in order):
 *  ----------------------------------------------------------------------
 * | HEADER | KEY(1) + RID(1) | KEY(2) + RID(2) | ... | KEY(n) + RID(n)
 *  ----------------------------------------------------------------------
 *
 *  Header format (size in byte, 16 bytes in total):
 *  ---------------------------------------------------------------------
 * | PageType (4) | CurrentSize (4) | MaxSize (4) |
 *  ---------------------------------------------------------------------
 *  -----------------------------------------------
 * |  NextPageId (4)
 *  -----------------------------------------------
 */

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
  void Init(int max_size);

  // helper methods
  auto GetNextPageId() const -> page_id_t;
  void SetNextPageId(page_id_t next_page_id);
  auto KeyAt(int index) const -> Tuple;
  auto ValueAt(int index) const -> Tuple;

  auto BinarySearch(const Tuple &key) const -> int;

  auto Insert(const Tuple &key, const Tuple &value) -> int;

  auto Split(const Tuple &key, const Tuple &value, BufferPoolManager *bpm,
             Tuple &key_to_insert, page_id_t &pid_to_insert) -> BasicPageGuard;

  auto Delete(const Tuple &key, BufferPoolManager *bpm, bool have_father)
      -> int;

 private:
  page_id_t next_page_id_;
  // Flexible array member for page data.
  char data_[0];
};
}  // namespace spdb
