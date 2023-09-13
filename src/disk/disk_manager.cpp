#include "disk/disk_manager.h"

namespace spdb {
DiskManager::DiskManager(const std::string& name) {
  db_name_ = name;
  db_file_.open(db_name_, std::ios::binary | std::ios::in | std::ios::out);
  if (!db_file_.is_open()) {
    db_file_.clear();
    db_file_.open(db_name_, std::ios::binary | std::ios::trunc | std::ios::out |
                                std::ios::in);
    if (!db_file_.is_open()) {
      throw std::runtime_error("can't open db file");
    }
  }
}

auto DiskManager::GetFileSize() -> size_t {
  db_file_.seekg(0, std::ios::end);
  auto size = db_file_.tellg();
  db_file_.seekg(0, std::ios::beg);
  return size;
}

void DiskManager::ReadPage(page_id_t id, char* data) {
  std::lock_guard<std::mutex> lock(latch_);
  size_t file_size = GetFileSize();
  size_t page_offset = id * PAGE_SIZE;
  if (page_offset >= file_size) {
    throw std::runtime_error("try reading out of the range of this file.");
  } else {
    db_file_.seekg(page_offset);
    db_file_.read(data, PAGE_SIZE);
    if (db_file_.bad()) {
      throw std::runtime_error("read error.");
    }
  }
}
void DiskManager::WritePage(page_id_t id, char* data) {
  std::lock_guard<std::mutex> lock(latch_);
  size_t file_size = GetFileSize();
  size_t page_offset = id * PAGE_SIZE;
  if (page_offset > file_size) {
    throw std::runtime_error("the page id should on order");
  } else {
    db_file_.seekp(page_offset);
    db_file_.write(data, PAGE_SIZE);
    if (db_file_.bad()) {
      throw std::runtime_error("write error.");
    }
    db_file_.flush();
  }
}
}  // namespace spdb