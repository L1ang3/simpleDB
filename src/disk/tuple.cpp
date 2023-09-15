#include "disk/tuple.h"

namespace spdb {
#define copy_from_data(dst, data, offset, type) \
  memcpy(dst, data + offset, sizeof(type));     \
  offset += sizeof(type);

#define copy_to_data(data, src, offset, type) \
  memcpy(data + offset, src, sizeof(type));   \
  offset += sizeof(type);

Tuple::Tuple(std::vector<Cloum>& cloums) {
  cloums_ = cloums;
  size_t data_size = 0;
  for (auto& col : cloums) {
    data_size += col.GetSize();
  }
  data_ = new char[data_size];
}

Tuple::~Tuple() { delete[] data_; }

void Tuple::SetValues(const char* src) {
  size_t data_size = 0;
  for (size_t i = 0; i < cloums_.size(); ++i) {
    data_size += cloums_[i].GetSize();
  }
  memcpy(data_, src, data_size);
}

void Tuple::SetRid(RID& rid) { rid_ = rid; }

auto Tuple::GetValueAt(size_t index) const -> char* {
  auto ret = (char*)malloc(cloums_[index].GetSize());
  size_t offset = 0;
  for (size_t i = 0; i < index; ++i) {
    offset += cloums_[i].GetSize();
  }
  memcpy(ret, data_ + offset, cloums_[index].GetSize());
  return ret;
}

template <class T>
auto Tuple::GetValueAtAs(size_t& index) const -> T* {
  auto src = GetValueAt(index);
  return reinterpret_cast<T*>(src);
}

bool Tuple::operator<(Tuple& other) {
  if (this->cloums_ != other.cloums_) {
    throw std::runtime_error("can not compare tuple with different type.");
  }
  size_t offset = 0;
  for (size_t i = 0; i < cloums_.size(); ++i) {
    auto ret_a = (char*)malloc(cloums_[i].GetSize());
    memcpy(ret_a, data_ + offset, cloums_[i].GetSize());
    auto ret_b = (char*)malloc(cloums_[i].GetSize());
    memcpy(ret_b, other.data_ + offset, cloums_[i].GetSize());
    int a = 0, b = 0;
    switch (cloums_[i].atr_.type_) {
      case CloumType::INT:
        a = *reinterpret_cast<int*>(ret_a);
        b = *reinterpret_cast<int*>(ret_b);
        if (a < b) {
          return true;
        } else if (a > b) {
          return false;
        }
        break;
      case CloumType::STRING:
        if (strcmp(ret_a, ret_b) < 0) {
          return true;
        } else if (strcmp(ret_a, ret_b) > 0) {
          return false;
        }
        break;
      default:
        break;
    }
    offset += cloums_[i].GetSize();
    free(ret_a);
    free(ret_b);
  }
  return false;
}

bool Tuple::operator>(Tuple& other) {
  if (this->cloums_ != other.cloums_) {
    throw std::runtime_error("can not compare tuple with different type.");
  }
  size_t offset = 0;
  for (size_t i = 0; i < cloums_.size(); ++i) {
    auto ret_a = (char*)malloc(cloums_[i].GetSize());
    memcpy(ret_a, data_ + offset, cloums_[i].GetSize());
    auto ret_b = (char*)malloc(cloums_[i].GetSize());
    memcpy(ret_b, other.data_ + offset, cloums_[i].GetSize());
    int a = 0, b = 0;
    switch (cloums_[i].atr_.type_) {
      case CloumType::INT:
        a = *reinterpret_cast<int*>(ret_a);
        b = *reinterpret_cast<int*>(ret_b);
        if (a > b) {
          return true;
        } else if (a < b) {
          return false;
        }
        break;
      case CloumType::STRING:
        if (strcmp(ret_a, ret_b) > 0) {
          return true;
        } else if (strcmp(ret_a, ret_b) < 0) {
          return false;
        }
        break;
      default:
        break;
    }
    offset += cloums_[i].GetSize();
    free(ret_a);
    free(ret_b);
  }
  return false;
}

bool Tuple::operator==(Tuple& other) {
  if (this->cloums_ != other.cloums_) {
    throw std::runtime_error("can not compare tuple with different type.");
  }
  size_t offset = 0;
  for (size_t i = 0; i < cloums_.size(); ++i) {
    auto ret_a = (char*)malloc(cloums_[i].GetSize());
    memcpy(ret_a, data_ + offset, cloums_[i].GetSize());
    auto ret_b = (char*)malloc(cloums_[i].GetSize());
    memcpy(ret_b, other.data_ + offset, cloums_[i].GetSize());
    int a = 0, b = 0;
    switch (cloums_[i].atr_.type_) {
      case CloumType::INT:
        a = *reinterpret_cast<int*>(ret_a);
        b = *reinterpret_cast<int*>(ret_b);
        if (a != b) {
          return false;
        }
        break;
      case CloumType::STRING:
        if (strcmp(ret_a, ret_b) != 0) {
          return false;
        }
        break;
      default:
        break;
    }
    offset += cloums_[i].GetSize();
    free(ret_a);
    free(ret_b);
  }
  return true;
}
}  // namespace spdb
