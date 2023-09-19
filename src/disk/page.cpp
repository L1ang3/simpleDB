#include "disk/page.h"
namespace spdb {

void Page::SetPageId(page_id_t id) { page_id_ = id; }

auto Page::GetData() -> char* { return data_; }

void Page::PinPage() { ++pin_count_; }

auto Page::GetPinCount() const -> int { return pin_count_; }

auto Page::GetPageId() -> page_id_t { return page_id_; }

void Page::ResetMemory() { memset(data_, 0, PAGE_SIZE); }

bool Page::IsDirty() { return is_dirty_; }

void Page::RUnlatch() { latch_.unlock_shared(); }

void Page::WUnlatch() { latch_.unlock(); }

void Page::RLatch() { latch_.lock_shared(); }

void Page::WLatch() { latch_.lock(); }
}  // namespace spdb