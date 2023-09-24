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
#include "table/b_plus_tree.h"

int main() {
  std::string prompt = "  > ";
  std::string waitline = "... ";
  std::cout << "Welcome!" << std::endl;
  while (true) {
    std::string query = "";
    std::cout << prompt;
    getline(std::cin, query);
    while (query.back() != ';' && query.back() != '.') {
      std::string str = "";
      std::cout << waitline;
      getline(std::cin, str);
      query += str;
    }

    if (query == "exit.") {
      return 0;
    }

    hsql::SQLParserResult result;
    hsql::SQLParser::parse(query, &result);
    auto tmp = result.size();
    if (result.isValid() && result.size() > 0) {
      spdb::Catalog catalog(CATALOG_NAME);
      const hsql::SQLStatement* statement = result.getStatement(0);

      if (statement->isType(hsql::kStmtSelect)) {
        const auto* select =
            static_cast<const hsql::SelectStatement*>(statement);
        auto ty = select->fromTable->type;
        auto tb = select->fromTable->getName();
        auto l = select->selectList;
        if (select->fromTable->hasSchema()) {
          auto sc = select->fromTable->schema;
        } else {
        }
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
              atr.type_ = spdb::CloumType::STRING;
              break;
            default:
              throw std::runtime_error("only support int&char type now.");
          }
          spdb::Cloum c{col->name, atr};
          value_type.emplace_back(c);
        }

        catalog.CreateTable(create->tableName, value_type, value_type);
      }
    } else {
      std::cerr << "unrecongized syntax." << std::endl;
    }
  }
}