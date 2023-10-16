#include "executor/value_executor.h"

namespace spdb {
ValueExecutor::ValueExecutor(Catalog *catalog, const hsql::SQLStatement *state)
    : AbstractExecutor(catalog) {
  if (!state->isType(hsql::StatementType::kStmtInsert)) {
    throw std::runtime_error(
        "ValueExecutor should construct with a insertstatement.");
  }

  auto insert = static_cast<const hsql::InsertStatement *>(state);
  if (!catalog->IsExisted(insert->tableName)) {
    throw std::runtime_error("table is not existed.");
  }

  table_info_ = catalog->GetTable(insert->tableName);
  size_t value_size{0};
  for (auto &c : table_info_.value_type_) {
    value_size += c.GetSize();
  }
  auto src = (char *)malloc(value_size);
  memset(src, '\0', value_size);
  size_t offset = 0;
  for (size_t i = 0; i < table_info_.value_type_.size(); ++i) {
    switch (table_info_.value_type_[i].GetType()) {
      case spdb::CloumType::CHAR:
        memcpy(src + offset, insert->values->at(i)->getName(),
               table_info_.value_type_[i].GetSize());
        offset += table_info_.value_type_[i].GetSize();
        break;
      case spdb::CloumType::INT:
        memcpy(src + offset, (char *)&insert->values->at(i)->ival, sizeof(int));
        offset += sizeof(int);
        break;
      default:
        throw std::runtime_error("only support int&char now.");
        break;
    }
  }

  Tuple t{table_info_.value_type_};
  t.SetValues(src);
  free(src);
  values_.push(t);
}

auto ValueExecutor::GetOutputCols() -> std::vector<Cloum> {
  return table_info_.value_type_;
}

ValueExecutor::~ValueExecutor() {}

auto ValueExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (values_.empty()) {
    return false;
  }
  *tuple = values_.front();
  values_.pop();
  return true;
}

}  // namespace spdb
