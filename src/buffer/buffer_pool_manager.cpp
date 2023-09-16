#include "buffer/buffer_pool_manager.h"

namespace spdb {

BufferPoolManager::BufferPoolManager(size_t pool_size,
                                     DiskManager *disk_manager,
                                     size_t replacer_k)
    : pool_size_(pool_size), disk_manager_(disk_manager) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }  // NOLINT

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  std::lock_guard<std::mutex> lock(latch_);

  // if there is no empty frames
  // evict an evictable page and free the frame.
  if (free_list_.empty()) {
    frame_id_t frame_tmp_id;
    if (replacer_->Evict(&frame_tmp_id)) {
      free_list_.push_back(frame_tmp_id);
      replacer_->Remove(frame_tmp_id);

      if (pages_[frame_tmp_id].IsDirty()) {
        disk_manager_->WritePage(pages_[frame_tmp_id].page_id_,
                                 pages_[frame_tmp_id].GetData());
      }

      pages_[frame_tmp_id].ResetMemory();
      page_table_.erase(pages_[frame_tmp_id].page_id_);
      pages_[frame_tmp_id].page_id_ = INVALID_PAGE_ID;
      pages_[frame_tmp_id].pin_count_ = 0;
      pages_[frame_tmp_id].is_dirty_ = false;

    } else {
      return nullptr;
    }
  }

  // put the page in the empty frame
  frame_id_t fid = free_list_.front();
  free_list_.pop_front();
  *page_id = AllocatePage();
  page_table_[*page_id] = fid;

  replacer_->RecordAccess(fid);
  replacer_->SetEvictable(fid, false);

  Page *new_page = &pages_[fid];
  new_page->pin_count_++;
  new_page->page_id_ = *page_id;
  new_page->is_dirty_ = false;

  // auto log = std::stringstream();
  // log << "thread " << std::this_thread::get_id() << " page "<< *page_id<<"
  // frame "<<fid; LOG_DEBUG("%s", log.str().c_str());
  return new_page;
}

auto BufferPoolManager::FetchPage(page_id_t page_id,
                                  [[maybe_unused]] AccessType access_type)
    -> Page * {
  if (page_id == INVALID_PAGE_ID) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(latch_);
  // auto log = std::stringstream();
  // log << "thread " << std::this_thread::get_id() << " page "<< page_id;
  // if the page is already in the buffer pool
  if (page_table_.find(page_id) != page_table_.end()) {
    Page *page_ret = &pages_[page_table_[page_id]];
    page_ret->pin_count_++;
    replacer_->SetEvictable(page_table_[page_id], false);

    // log<<" frame "<<page_table_[page_id];
    // LOG_DEBUG("%s", log.str().c_str());
    return page_ret;
  }

  // to put the page in the frame
  if (free_list_.empty()) {
    frame_id_t frame_tmp_id;
    if (replacer_->Evict(&frame_tmp_id)) {
      free_list_.push_back(frame_tmp_id);
      replacer_->Remove(frame_tmp_id);

      if (pages_[frame_tmp_id].IsDirty()) {
        disk_manager_->WritePage(pages_[frame_tmp_id].page_id_,
                                 pages_[frame_tmp_id].GetData());
      }

      pages_[frame_tmp_id].ResetMemory();
      page_table_.erase(pages_[frame_tmp_id].page_id_);
      pages_[frame_tmp_id].page_id_ = INVALID_PAGE_ID;
      pages_[frame_tmp_id].pin_count_ = 0;
      pages_[frame_tmp_id].is_dirty_ = false;

    } else {
      return nullptr;
    }
  }

  frame_id_t fid = free_list_.front();
  free_list_.pop_front();
  page_table_[page_id] = fid;
  replacer_->RecordAccess(fid);
  replacer_->SetEvictable(fid, false);

  Page *new_page = &pages_[fid];
  new_page->pin_count_++;
  new_page->page_id_ = page_id;
  disk_manager_->ReadPage(page_id, new_page->GetData());
  new_page->is_dirty_ = false;

  // log<<" fetch from disk, fid "<<fid;
  // LOG_DEBUG("%s", log.str().c_str());
  return new_page;
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty,
                                  [[maybe_unused]] AccessType access_type)
    -> bool {
  std::lock_guard<std::mutex> lock(latch_);

  if (page_table_.find(page_id) == page_table_.end() ||
      pages_[page_table_[page_id]].GetPinCount() <= 0) {
    return false;
  }

  frame_id_t fid = page_table_[page_id];
  pages_[fid].pin_count_--;
  if (pages_[fid].GetPinCount() == 0) {
    replacer_->SetEvictable(fid, true);
  }
  pages_[fid].is_dirty_ = is_dirty ? true : pages_[fid].is_dirty_;

  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);

  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }

  frame_id_t fid = page_table_[page_id];
  disk_manager_->WritePage(page_id, pages_[fid].GetData());
  pages_[fid].is_dirty_ = false;

  // auto log = std::stringstream();
  // log << "thread " << std::this_thread::get_id() << " flush page "<< page_id;
  // LOG_DEBUG("%s", log.str().c_str());
  return true;
}

void BufferPoolManager::FlushAllPages() {
  std::lock_guard<std::mutex> lock(latch_);
  for (auto [pid, fid] : page_table_) {
    disk_manager_->WritePage(pid, pages_[fid].GetData());
    pages_[fid].is_dirty_ = false;
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  // auto log = std::stringstream();
  // log << "thread " << std::this_thread::get_id() << " delete page "<<
  // page_id; LOG_DEBUG("%s", log.str().c_str());
  std::lock_guard<std::mutex> lock(latch_);
  if (page_table_.find(page_id) == page_table_.end()) {
    return true;
  }

  frame_id_t fid = page_table_[page_id];

  if (pages_[fid].GetPinCount() > 0) {
    return false;
  }

  if (pages_[fid].IsDirty()) {
    disk_manager_->WritePage(page_id, pages_[fid].GetData());
  }
  DeallocatePage(page_id);
  replacer_->Remove(fid);
  free_list_.push_back(fid);
  pages_[fid].ResetMemory();
  pages_[fid].page_id_ = INVALID_PAGE_ID;
  page_table_.erase(page_id);

  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t {
  page_id_t page_tmp_id;
  if (page_id_bin_.empty()) {
    page_tmp_id = next_page_id_++;
  } else {
    page_tmp_id = page_id_bin_.back();
    page_id_bin_.pop_back();
  }
  return page_tmp_id;
}

void BufferPoolManager::DeallocatePage(page_id_t page_id) {
  page_id_bin_.push_back(page_id);
}

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard {
  Page *page = FetchPage(page_id);
  return {this, page};
}

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  Page *page = FetchPage(page_id);
  if (page != nullptr) {
    page->RLatch();
  }
  return {this, page};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  Page *page = FetchPage(page_id);
  if (page != nullptr) {
    page->WLatch();
  }
  return {this, page};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard {
  Page *page = NewPage(page_id);
  return {this, page};
}

}  // namespace spdb
