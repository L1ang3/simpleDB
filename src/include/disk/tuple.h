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
  RID rid_{};

 public:
  explicit Tuple(std::vector<Cloum>&);
  ~Tuple();

  void SetValues(const char* src);
  void SetRid(RID&);
  auto GetValueAt(int) const -> char*;

  template <class T>
  auto GetValueAtAs(int index) const -> T* {
    auto src = GetValueAt(index);
    return reinterpret_cast<T*>(src);
  }

  bool operator<(Tuple& other);
  bool operator>(Tuple& other);
  bool operator==(Tuple& other);
};
}  // namespace spdb