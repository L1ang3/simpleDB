#pragma once

#include <cstdint>
#include <string>

namespace spdb {
#define page_id_t int32_t
#define slot_id_t int32_t
#define INVALID_PAGE_ID -1
#define PAGE_SIZE 4096
#define LRUK_REPLACER_K 5

class RID {
 private:
  page_id_t pid_;
  slot_id_t sid_;

 public:
  explicit RID(page_id_t pid, slot_id_t sid) : pid_(pid), sid_(sid) {}

  auto GetPageId() const -> page_id_t { return pid_; }
  auto GetSlotId() const -> slot_id_t { return sid_; }
};

enum class CloumType { INVALID, INT, BOOL };

class Cloum {
 public:
  std::string cloum_name_;
  CloumType type_;

  explicit Cloum(std::string& name, CloumType type)
      : cloum_name_(name), type_(type) {}
};
}  // namespace spdb