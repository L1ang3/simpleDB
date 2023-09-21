#pragma once
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
  Tuple(const Tuple& other);

  void SetValues(char* src);
  void SetRid(RID&);
  auto GetValueAt(int) const -> char*;

  auto GetData() const -> const char*;

  template <class T>
  auto GetValueAtAs(int index) const -> T* {
    auto src = GetValueAt(index);
    return reinterpret_cast<T*>(src);
  }

  Tuple& operator=(const Tuple&);
  bool operator<(Tuple other) const;
  bool operator>(Tuple other) const;
  bool operator==(Tuple other) const;
};
}  // namespace spdb