#include <memory.h>

#include <stdexcept>
#include <vector>

#include "config/config.h"

namespace spdb {
class Tuple {
 private:
  char* data_;
  std::vector<Cloum> cloums_;
  RID rid_;

 public:
  auto GetCloums() const -> std::vector<int>;
  auto GetCloumAt(size_t) const -> int;
};
}  // namespace spdb