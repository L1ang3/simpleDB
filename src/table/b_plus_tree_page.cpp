#include "table/b_plus_tree_page.h"
namespace spdb {
/*
 * Helper methods to get/set page type
 * Page type enum class is defined in b_plus_tree_page.h
 */
auto BPlusTreePage::IsLeafPage() const -> bool {
  return page_type_ == PageType::Leaf;
}
void BPlusTreePage::SetPageType(PageType page_type) { page_type_ = page_type; }

/*
 * Helper methods to get/set size (number of key/value pairs stored in that
 * page)
 */
auto BPlusTreePage::GetSize() const -> int { return size_; }
void BPlusTreePage::SetSize(int size) { size_ = size; }
void BPlusTreePage::IncreaseSize(int amount) { size_ += amount; }

/*
 * Helper methods to get/set max size (capacity) of the page
 */
auto BPlusTreePage::GetMaxSize() const -> int { return max_size_; }
void BPlusTreePage::SetMaxSize(int size) { max_size_ = size; }

/*
 * Helper method to get min page size
 * Generally, min page size == max page size / 2
 */
auto BPlusTreePage::GetMinSize() const -> int {
  return IsLeafPage() ? max_size_ / 2 : max_size_ / 2 + max_size_ % 2;
}

auto BPlusTreePage::GetValueSize() const -> size_t { return value_size_; }
void BPlusTreePage::SetValueSize(size_t value_size) {
  value_size_ = value_size;
}
auto BPlusTreePage::GetKeySize() const -> size_t { return key_size_; }
void BPlusTreePage::SetKeySize(size_t key_size) { key_size_ = key_size; }
}  // namespace spdb
