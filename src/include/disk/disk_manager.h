#pragma once

#include <fstream>
#include <mutex>
#include <string>

#include "config/config.h"
namespace spdb {
class DiskManager {
 private:
  std::fstream db_file_;
  std::string db_name_;
  std::mutex latch_;

 public:
  DiskManager(std::string& name);

  void ReadPage(page_id_t id, char* data);

  void WritePage(page_id_t id, char* data);

  auto GetFileSize() -> size_t;
};
}  // namespace spdb