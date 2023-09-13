#include "disk/page.h"
namespace spdb {

void Page::SetPageId(page_id_t id) { id_ = id; }

auto Page::GetData() const -> char* { return data_; }

void Page::PinPage() { ++pin_count_; }

auto Page::GetPinCount() const -> int { return pin_count_; }

void Page::ResetMemory() { memset(data_, 0, PAGE_SIZE); }
}  // namespace spdb