#pragma once

#include <memory.h>

#include "config/config.h"

namespace spdb {
class Page {
 private:
  page_id_t id_{INVALID_PAGE_ID};
  int pin_count_{0};
  char *data_;

 public:
  Page() {
    data_ = new char[PAGE_SIZE];
    ResetMemory();
  }
  ~Page() { delete[] data_; }

  void SetPageId(page_id_t);

  auto GetData() const -> char *;

  void PinPage();

  auto GetPinCount() const -> int;

  void ResetMemory();
};
}  // namespace spdb