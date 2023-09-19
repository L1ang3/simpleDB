#pragma once
#include <cstddef>
namespace spdb {
enum class PageType { Leaf, Internal };

class BPlusTreePage {
 public:
  // Delete all constructor / destructor to ensure memory safety
  BPlusTreePage() = delete;
  BPlusTreePage(const BPlusTreePage &other) = delete;
  ~BPlusTreePage() = delete;

  auto IsLeafPage() const -> bool;
  void SetPageType(PageType page_type);

  auto GetSize() const -> int;
  void SetSize(int size);
  void IncreaseSize(int amount);

  auto GetMaxSize() const -> int;
  void SetMaxSize(int max_size);
  auto GetMinSize() const -> int;

  auto GetValueSize() const -> size_t;
  void SetValueSize(size_t);
  auto GetKeySize() const -> size_t;
  void SetKeySize(size_t);

 private:
  // member variable, attributes that both internal and leaf page share
  PageType page_type_;
  int size_;
  int max_size_;
  size_t key_size_;
  size_t value_size_;
};

}  // namespace spdb
