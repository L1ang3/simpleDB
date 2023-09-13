#include "disk/page_guard.h"

#include "buffer/buffer_pool_manager.h"
namespace spdb {
BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept {
  if (bpm_ != nullptr) {
    this->Drop();
  }
  bpm_ = that.bpm_;
  page_ = that.page_;
  is_dirty_ = that.is_dirty_;
  that.bpm_ = nullptr;
  that.page_ = nullptr;
}

void BasicPageGuard::Drop() {
  if (bpm_ != nullptr && page_ != nullptr) {
    if (is_dirty_) {
      if (bpm_->FlushPage(page_->GetPageId())) {
        is_dirty_ = false;
      } else {
        std::cout << "flush fail\n";
      }
    }
    bpm_->UnpinPage(page_->GetPageId(), false);
  }
  bpm_ = nullptr;
  page_ = nullptr;
}

auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept
    -> BasicPageGuard & {
  this->Drop();
  bpm_ = that.bpm_;
  page_ = that.page_;
  is_dirty_ = that.is_dirty_;
  that.bpm_ = nullptr;
  that.page_ = nullptr;
  return *this;
}

BasicPageGuard::~BasicPageGuard() {
  if (bpm_ != nullptr && page_ != nullptr) {
    if (is_dirty_) {
      if (bpm_->FlushPage(page_->GetPageId())) {
        is_dirty_ = false;
      } else {
        std::cout << "flush fail\n";
      }
    }
    bpm_->UnpinPage(page_->GetPageId(), false);
  }
  bpm_ = nullptr;
  page_ = nullptr;
};  // NOLINT

ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept = default;

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept
    -> ReadPageGuard & {
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
  }
  guard_.Drop();
  guard_ = std::move(that.guard_);
  return *this;
}

void ReadPageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
  }
  guard_.Drop();
}

ReadPageGuard::~ReadPageGuard() {
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
  }
}  // NOLINT

WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept {
  this->Drop();
  guard_ = std::move(that.guard_);
}

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept
    -> WritePageGuard & {
  this->Drop();
  guard_ = std::move(that.guard_);
  return *this;
}

void WritePageGuard::Drop() {
  if (guard_.bpm_ != nullptr && guard_.page_ != nullptr) {
    if (guard_.is_dirty_) {
      if (guard_.bpm_->FlushPage(guard_.page_->GetPageId())) {
        guard_.is_dirty_ = false;
      } else {
        std::cout << "flush fail\n";
      }
    }
    guard_.bpm_->UnpinPage(guard_.page_->GetPageId(), false);
    guard_.page_->WUnlatch();
  }
  guard_.bpm_ = nullptr;
  guard_.page_ = nullptr;
  guard_.is_dirty_ = false;
}

WritePageGuard::~WritePageGuard() {
  if (guard_.bpm_ != nullptr && guard_.page_ != nullptr) {
    if (guard_.is_dirty_) {
      if (guard_.bpm_->FlushPage(guard_.page_->GetPageId())) {
        guard_.is_dirty_ = false;
      } else {
        std::cout << "flush fail\n";
      }
    }
    guard_.bpm_->UnpinPage(guard_.page_->GetPageId(), false);
    guard_.page_->WUnlatch();
  }
  guard_.bpm_ = nullptr;
  guard_.page_ = nullptr;
  guard_.is_dirty_ = false;
}  // NOLINT
}  // namespace spdb