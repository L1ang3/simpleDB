#include <memory.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "config/config.h"

namespace spdb {
class Tuple {
 private:
  char* data_;
  std::vector<Cloum> cloums_;
  RID rid_;

 public:
  explicit Tuple(std::vector<Cloum>&);
  ~Tuple();

  void SetValues(const char* src);
  void SetRid(RID&);
  auto GetValueAt(size_t) const -> char*;

  template <class T>
  auto GetValueAtAs(size_t&) const -> T*;

  bool operator<(Tuple& other);
  bool operator>(Tuple& other);
  bool operator==(Tuple& other);
};
}  // namespace spdb