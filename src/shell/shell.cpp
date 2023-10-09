#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "SQLParser.h"
#include "buffer/buffer_pool_manager.h"
#include "config/catalog.h"
#include "config/config.h"
#include "disk/tuple.h"
#include "executor/seq_scan_executor.h"
#include "table/b_plus_tree.h"

class TableWriter {
 public:
  TableWriter() = default;

  void AddHeader(std::vector<std::string> headers) {
    headers_.reserve(headers.size());
    max_.reserve(headers.size());
    for (auto& header : headers) {
      headers_.emplace_back(header);
      max_.emplace_back(header.length());
    }
  }

  void AddRow(std::vector<std::string>& row) {
    if (row.size() != headers_.size()) {
      throw std::runtime_error(
          "the row size of table should equal to headers size.");
    }
    for (size_t i = 0; i < row.size(); ++i) {
      max_[i] = max_[i] >= row[i].length() ? max_[i] : row[i].length();
    }
    rows_.push_back(row);
  }

  void DrawLine() {
    for (size_t i = 0; i < max_.size(); ++i) {
      std::cout << "+-";
      for (size_t j = 0; j <= max_[i]; ++j) {
        std::cout << '-';
      }
    }
    std::cout << '+' << std::endl;
  }

  void DrawTable() {
    DrawLine();
    for (size_t i = 0; i < headers_.size(); ++i) {
      std::cout << "| " << std::setw(max_[i]) << setiosflags(std::ios::left)
                << std::setfill(' ') << headers_[i] << ' ';
    }
    std::cout << '|' << std::endl;
    if (rows_.size() != 0) {
      DrawLine();
    }
    for (size_t i = 0; i < rows_.size(); ++i) {
      for (size_t j = 0; j < headers_.size(); ++j) {
        std::cout << "| " << std::setw(max_[j]) << setiosflags(std::ios::left)
                  << std::setfill(' ');
        std::cout << rows_[i][j] << ' ';
      }
      std::cout << '|' << std::endl;
    }
    DrawLine();
  }

 private:
  std::vector<std::string> headers_;
  std::vector<std::vector<std::string>> rows_;
  std::vector<int> max_;
};

int main() {
  spdb::Catalog catalog(CATALOG_NAME);
  std::string prompt = "  > ";
  std::string waitline = "... ";
  std::cout << "Welcome!" << std::endl;
  while (true) {
    std::string query = "";
    std::cout << prompt;
    getline(std::cin, query);
    while (query.back() != ';') {
      std::string str = "";
      std::cout << waitline;
      getline(std::cin, str);
      query += str;
    }

    if (query == "show tables;") {
      TableWriter writer;
      writer.AddHeader({"name", "cols"});

      for (auto& table : catalog.GetTables()) {
        std::vector<std::string> row{};
        row.push_back(table.disk_name_);
        std::string str = "(";
        for (auto& col : table.value_type_) {
          str += col.cloum_name_;
          str += ":";
          switch (col.atr_.type_) {
            case spdb::CloumType::INT:
              str += "INT ";
              break;
            case spdb::CloumType::CHAR:
              str += "CHAR ";
              break;
            case spdb::CloumType::BOOL:
              str += "BOOL ";
              break;
            default:
              throw std::runtime_error("only support bool&int&char now.");
          }
        }
        str += ")";
        row.push_back(str);
        writer.AddRow(row);
      }
      writer.DrawTable();
      continue;
    }
    if (query == "exit;") {
      return 0;
    }

    hsql::SQLParserResult result;
    hsql::SQLParser::parse(query, &result);
    auto tmp = result.size();
    if (result.isValid() && result.size() > 0) {
      const hsql::SQLStatement* statement = result.getStatement(0);

      if (statement->isType(hsql::kStmtSelect)) {
        const auto* select =
            static_cast<const hsql::SelectStatement*>(statement);

        for (auto& c : *select->selectList) {
          auto p = c->getName();

          std::cout << c->ival << std::endl;
        }
        auto table = *select;
      } else if (statement->isType(hsql::kStmtCreate)) {
        const auto* create =
            static_cast<const hsql::CreateStatement*>(statement);

        if (catalog.IsExisted(create->tableName)) {
          std::cerr << "table " << create->tableName << " is already existed."
                    << std::endl;
          continue;
        }

        auto cols = *create->columns;

        // To make sure the key is unique, use value as key here.
        std::vector<spdb::Cloum> value_type;
        value_type.reserve(cols.size());
        for (auto& col : cols) {
          spdb::CloumAtr atr{};
          switch (col->type.data_type) {
            case hsql::DataType::INT:
              atr.size_ = sizeof(int);
              atr.type_ = spdb::CloumType::INT;
              break;
            case hsql::DataType::CHAR:
              atr.size_ = col->type.length;
              atr.type_ = spdb::CloumType::CHAR;
              break;
            default:
              throw std::runtime_error("only support int&char type now.");
          }
          spdb::Cloum c{col->name, atr};
          value_type.emplace_back(c);
        }

        catalog.CreateTable(create->tableName, value_type, value_type);
      } else if (statement->isType(hsql::kStmtDrop)) {
        const auto* drop = static_cast<const hsql::DropStatement*>(statement);
        if (drop->type == hsql::DropType::kDropTable) {
          if (!catalog.IsExisted(drop->name)) {
            std::cerr << "table is not existed." << std::endl;
            continue;
          }
          if (catalog.DropTable(drop->name)) {
            std::cout << "successfully drop." << std::endl;
          } else {
            std::cerr << "fail to drop." << std::endl;
          }
        } else {
          std::cerr << "only support table now." << std::endl;
        }
      }
    } else {
      std::cerr << "unrecongized syntax." << std::endl;
    }
  }
  return 0;
}