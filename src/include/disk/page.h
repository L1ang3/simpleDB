#pragma once

#include <memory.h>

#include <shared_mutex>

#include "config/config.h"

namespace spdb {

class Page {
  friend class BufferPoolManager;

 private:
  page_id_t page_id_{INVALID_PAGE_ID};
  int pin_count_{0};
  char *data_;
  bool is_dirty_{false};
  std::shared_mutex latch_;

 public:
  Page() {
    data_ = new char[PAGE_SIZE];
    ResetMemory();
  }
  ~Page() { delete[] data_; }

  void SetPageId(page_id_t);

  auto GetData() -> char *;

  void PinPage();

  auto GetPinCount() const -> int;

  auto GetPageId() -> page_id_t;

  void ResetMemory();

  bool IsDirty();

  void RUnlatch();

  void WUnlatch();

  void RLatch();

  void WLatch();
};

}  // namespace spdb