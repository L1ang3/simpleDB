#include "executor/value_executor.h"

namespace spdb {
ValueExecutor::ValueExecutor(Catalog *catalog, hsql::SQLStatement *state)
    : AbstractExecutor(catalog) {
  if (!state->isType(hsql::StatementType::kStmtInsert)) {
    throw std::runtime_error(
        "ValueExecutor should construct with a insertstatement.");
  }

  auto insert = static_cast<const hsql::InsertStatement *>(state);
  if (!catalog->IsExisted(insert->tableName)) {
    throw std::runtime_error("table is not existed.");
  }

  auto table_info = catalog->GetTable(insert->tableName);
  size_t value_size{0};
  for (auto &c : table_info.value_type_) {
    value_size += c.GetSize();
  }
  auto src = (char *)malloc(value_size);
  memset(src, 0, value_size);
  size_t offset = 0;
  for (size_t i = 0; i < table_info.value_type_.size(); ++i) {
    switch (insert->values->at(i)->columnType.data_type) {
      case hsql::DataType::CHAR:
        memcpy(src + offset, insert->columns->at(i),
               insert->values->at(i)->columnType.length);
        offset += insert->values->at(i)->columnType.length;
        break;
      case hsql::DataType::INT:
        memcpy(src + offset, insert->columns->at(i), sizeof(int));
        offset += sizeof(int);
        break;
      default:
        throw std::runtime_error("only support int&char now.");
        break;
    }
  }

  Tuple t{table_info.value_type_};
  t.SetValues(src);
  free(src);
}

ValueExecutor::~ValueExecutor() {}

auto ValueExecutor::Next(Tuple *tuple, RID *rid) -> bool {}

}  // namespace spdb
