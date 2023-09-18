#pragma once

#include <queue>
#include <string>

#include "buffer/buffer_pool_manager.h"
#include "config/config.h"
#include "disk/page_guard.h"
#include "disk/tuple.h"
#include "table/b_plus_tree_page.h"

namespace spdb {

#define INTERNAL_PAGE_HEADER_SIZE 12
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
  void Init(int max_size);

  /**
   * @param index The index of the key to get. Index must be non-zero.
   * @return Key at index
   */
  auto KeyAt(int index) const -> Tuple;

  /**
   *
   * @param index The index of the key to set. Index must be non-zero.
   * @param key The new value for key
   */
  void SetKeyAt(int index, const Tuple &key);

  void SetValueAt(int index, const page_id_t &value);

  /**
   *
   * @param index the index
   * @return the value at the index
   */
  auto ValueAt(int index) const -> page_id_t;

  auto BinarySearch(const Tuple &key) const -> int {
    // int l = 1;
    // int r = GetSize() - 1;
    // if (r == 0) {
    //   return r;
    // }
    // int ret = 0;

    // while (true) {
    //   int m = (l + r) / 2;
    //   if (cmp(key, array_[m].first) == 1) {
    //     if (m + 1 >= GetSize() || cmp(key, array_[m + 1].first) == -1) {
    //       ret = m;
    //       break;
    //     }
    //     l = m + 1;
    //   } else if (cmp(key, array_[m].first) == -1) {
    //     if (m - 1 <= 0 || cmp(key, array_[m - 1].first) == 1) {
    //       ret = m - 1;
    //       break;
    //     }
    //     r = m - 1;
    //   } else {
    //     ret = m;
    //     break;
    //   }
    // }
    // return ret;
    return 0;
  }

  auto Insert(const Tuple &key, const page_id_t &value) -> int {
    int ret = 1;

    // if (GetSize() < GetMaxSize()) {
    //   for (int i = GetSize() - 1; i >= 0; i--) {
    //     if (i == 0) {
    //       array_[i + 1].first = key;
    //       array_[i + 1].second = value;
    //       IncreaseSize(1);
    //       break;
    //     }

    //     if (cmp(key, array_[i].first) == -1) {
    //       array_[i + 1] = array_[i];
    //     } else {
    //       array_[i + 1].first = key;
    //       array_[i + 1].second = value;
    //       IncreaseSize(1);
    //       break;
    //     }
    //   }
    //   ret = 0;
    // }
    return ret;
  }

  auto Split(const Tuple &key, const page_id_t &value, BufferPoolManager *bpm,
             Tuple &key_to_insert, page_id_t &pid_to_insert) -> BasicPageGuard;

  auto Delete(const Tuple &key, BufferPoolManager *bpm, bool have_father)
      -> int;

 private:
  // Flexible array member for page data.
  char data_[0];
};
}  // namespace spdb
