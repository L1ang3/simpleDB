#pragma once
#include <fstream>
#include <string>

#include "buffer/buffer_pool_manager.h"
#include "config.h"
#include "table/b_plus_tree.h"
namespace spdb {
class TableInfo {
 public:
  std::string disk_name_;
  std::vector<Cloum> key_type_;
  std::vector<Cloum> value_type_;
  page_id_t root_id_;
  int leaf_max_size_;
  int internal_max_size_;
};

class Catalog {
  /*
   *  Catalog:
   *  ------------------------------------------------------------
   * | TableSize (8) | TableInfo(1) | ... | TableInfo(n) |
   *  ------------------------------------------------------------
   *  TableInfo:
   *  ------------------------------------------------------------
   * |  DiskNameLength (8) | DiskName (DiskNameLength) | KeyNums (8) |
   *  ------------------------------------------------------------
   * |  Key1Size (8) | Key1 (Key1Size) | ... | ValueNums (8) |
   *  ------------------------------------------------------------
   * |  Value1Size (8) | Value1 (Value1Size) | ... | HeaderId (4) |
   *  ------------------------------------------------------------
   * |  LeafMaxSize (4) | InternalMaxSize (4)
   *  ------------------------------------------------------------
   */
 public:
  explicit Catalog(std::string name) {
    catal_name_ = name;
    std::fstream catal_file(name, std::ios::in | std::ios::binary);

    size_t tsize = 0;
    if (catal_file.is_open()) {
      catal_file.read((char *)&tsize, sizeof(size_t));
    }
    tables_.reserve(tsize);
    for (size_t i = 0; i < tsize; ++i) {
      size_t disk_name_length = 0;
      catal_file.read((char *)&disk_name_length, sizeof(size_t));
      auto src = (char *)malloc(disk_name_length);
      catal_file.read(src, disk_name_length);
      std::string disk_name = src;
      free(src);

      size_t key_nums = 0;
      catal_file.read((char *)&key_nums, sizeof(size_t));
      std::vector<Cloum> key_type;
      key_type.reserve(key_nums);
      for (size_t j = 0; j < key_nums; ++j) {
        CloumAtr atr{};
        size_t key_name_length = 0;
        catal_file.read((char *)&key_name_length, sizeof(size_t));
        auto src = (char *)malloc(key_name_length);
        catal_file.read(src, key_name_length);
        catal_file.read((char *)&atr, sizeof(CloumAtr));

        Cloum c{src, atr};
        key_type.emplace_back(c);
        free(src);
      }

      size_t val_nums = 0;
      catal_file.read((char *)&val_nums, sizeof(size_t));
      std::vector<Cloum> value_type;
      value_type.reserve(val_nums);
      for (size_t j = 0; j < val_nums; ++j) {
        CloumAtr atr{};
        size_t val_name_length = 0;
        catal_file.read((char *)&val_name_length, sizeof(size_t));
        auto src = (char *)malloc(val_name_length);
        catal_file.read(src, val_name_length);
        catal_file.read((char *)&atr, sizeof(CloumAtr));

        Cloum c{src, atr};
        value_type.emplace_back(c);
        free(src);
      }
      page_id_t header_id = INVALID_PAGE_ID;
      int leaf_max_size = 0;
      int internal_max_size = 0;
      catal_file.read((char *)&header_id, sizeof(page_id_t));
      catal_file.read((char *)&leaf_max_size, sizeof(int));
      catal_file.read((char *)&internal_max_size, sizeof(int));
      TableInfo table_info{disk_name, key_type,      value_type,
                           header_id, leaf_max_size, internal_max_size};
      tables_.push_back(table_info);
    }

    catal_file.close();
  }

  ~Catalog() {
    std::fstream catal_file(catal_name_,
                            std::ios::out | std::ios::binary | std::ios::trunc);
    size_t tsize = tables_.size();
    catal_file.write((char *)&tsize, sizeof(size_t));
    for (size_t i = 0; i < tsize; ++i) {
      size_t disk_name_length = tables_[i].disk_name_.length() + 1;
      catal_file.write((char *)&disk_name_length, sizeof(size_t));
      catal_file.write(tables_[i].disk_name_.data(), disk_name_length);

      size_t key_nums = tables_[i].key_type_.size();
      catal_file.write((char *)&key_nums, sizeof(size_t));
      for (size_t j = 0; j < key_nums; ++j) {
        size_t key_name_length =
            tables_[i].key_type_[j].cloum_name_.length() + 1;
        catal_file.write((char *)&key_name_length, sizeof(size_t));
        catal_file.write(tables_[i].key_type_[j].cloum_name_.data(),
                         key_name_length);
        catal_file.write((char *)&tables_[i].key_type_[j].atr_,
                         sizeof(CloumAtr));
      }

      size_t val_nums = tables_[i].value_type_.size();
      catal_file.write((char *)&val_nums, sizeof(size_t));
      for (size_t j = 0; j < val_nums; ++j) {
        size_t val_name_length =
            tables_[i].value_type_[j].cloum_name_.length() + 1;
        catal_file.write((char *)&val_name_length, sizeof(size_t));
        catal_file.write(tables_[i].value_type_[j].cloum_name_.data(),
                         val_name_length);
        catal_file.write((char *)&tables_[i].value_type_[j].atr_,
                         sizeof(CloumAtr));
      }

      catal_file.write((char *)&tables_[i].root_id_, sizeof(page_id_t));
      catal_file.write((char *)&tables_[i].leaf_max_size_, sizeof(int));
      catal_file.write((char *)&tables_[i].internal_max_size_, sizeof(int));
    }
  }

  bool CreateTable(std::string name, std::vector<Cloum> key_type,
                   std::vector<Cloum> value_type) {
    for (auto &table : tables_) {
      if (name == table.disk_name_) {
        return false;
      }
    }
    auto disk = DiskManager(name);
    // create and fetch header_page
    BufferPoolManager bpm(5, &disk);
    // create b+ tree
    size_t kv_size = 0;
    for (auto &col : key_type) {
      kv_size += col.GetSize();
    }
    for (auto &col : value_type) {
      kv_size += col.GetSize();
    }

    int leaf_max_size = (PAGE_SIZE - LEAF_HEADER_SIZE) / kv_size;
    int internal_max_size = (PAGE_SIZE - INTERNAL_HEADER_SIZE) / kv_size;
    TableInfo table{name,          key_type,
                    value_type,    INVALID_PAGE_ID,
                    leaf_max_size, internal_max_size};
    tables_.push_back(table);
    BPlusTree(&bpm, key_type, value_type, leaf_max_size, internal_max_size);
    return true;
  }
  TableInfo GetTable(std::string name) {
    for (auto &table : tables_) {
      if (table.disk_name_ == name) {
        return table;
      }
    }
    return {};
  }

  void ModifyTableRoot(std::string name, page_id_t root_id) {
    for (auto &table : tables_) {
      if (table.disk_name_ == name) {
        table.root_id_ = root_id;
        break;
      }
    }
  }

  bool DropTable(std::string name) {
    for (auto it = tables_.begin(); it != tables_.end(); ++it) {
      if (it->disk_name_ == name) {
        if (!std::remove(name.data()) == 0) {
          return false;
        } else {
          tables_.erase(it);
        }
        break;
      }
    }
    return true;
  }
  auto GetTables() -> std::vector<TableInfo> { return tables_; }

  bool IsExisted(std::string name) {
    for (auto &table : tables_) {
      if (table.disk_name_ == name) {
        return true;
      }
    }
    return false;
  }

 private:
  std::string catal_name_;
  std::vector<TableInfo> tables_;
};
}  // namespace spdb
