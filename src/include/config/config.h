#pragma once

#include <cstdint>
#include <string>

namespace spdb {
#define page_id_t int32_t
#define slot_id_t int32_t
#define INVALID_PAGE_ID -1
#define PAGE_SIZE 4096

#define BUFFER_POOL_SIZE 50
#define LRUK_REPLACER_K 5
#define CATALOG_NAME "catalog.db"

class RID {
 private:
  page_id_t pid_{-1};
  slot_id_t sid_{-1};

 public:
  explicit RID(page_id_t pid, slot_id_t sid) : pid_(pid), sid_(sid) {}

  RID() {}

  auto GetPageId() const -> page_id_t { return pid_; }
  auto GetSlotId() const -> slot_id_t { return sid_; }
};

enum class CloumType { INVALID, INT, BOOL, CHAR };

struct CloumAtr {
  CloumType type_;
  size_t size_;
};

class Cloum {
 public:
  std::string cloum_name_;
  CloumAtr atr_;

  explicit Cloum(std::string name, CloumAtr atr)
      : cloum_name_(name), atr_(atr) {}

  auto GetSize() const -> size_t { return atr_.size_; }

  auto GetType() const -> CloumType { return atr_.type_; }

  bool operator==(Cloum other) const {
    return cloum_name_ == other.cloum_name_ && atr_.size_ == other.atr_.size_ &&
           atr_.type_ == atr_.type_;
  }

  bool operator!=(Cloum other) const {
    return !(cloum_name_ == other.cloum_name_ &&
             atr_.size_ == other.atr_.size_ && atr_.type_ == atr_.type_);
  }
};

}  // namespace spdb