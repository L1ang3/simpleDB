#include "disk/tuple.h"

namespace spdb {
#define copy_to_dst(dst, src, offset, type) \
  memcpy(dst, src + offset, sizeof(type));  \
  offset += sizeof(type)

auto Tuple::GetCloumAt(size_t index) const -> int {
  auto cloums = GetCloums();
  if (index >= cloums.size() || index < 0) {
    throw std::runtime_error("fail accessing.");
  }
  return cloums[index];
}

auto Tuple::GetCloums() const -> std::vector<int> {
  std::vector<int> result;
  size_t offset = 0;
  int tmp = 0;
  for (auto &col : cloums_) {
    switch (col.type_) {
      case CloumType::INT:
        copy_to_dst(&tmp, data_ + offset, offset, int);
        result.emplace_back(tmp);
        break;
      case CloumType::BOOL:
        copy_to_dst(&tmp, data_ + offset, offset, bool);
        result.emplace_back(tmp);
        break;
      default:
        throw std::runtime_error("other types are not implented");
        break;
    }
  }
  return result;
}
}  // namespace spdb
