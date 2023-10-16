#include "executor/projection_executor.h"

namespace spdb {
int GetIndexByName(std::string name, std::vector<Cloum> cols) {
  for (size_t i = 0; i < cols.size(); ++i) {
    if (name == cols[i].cloum_name_) {
      return i;
    }
  }
  return -1;
}

ProjectionExecutor::ProjectionExecutor(Catalog *catalog,
                                       const hsql::SQLStatement *state)
    : AbstractExecutor(catalog) {
  if (!state->isType(hsql::StatementType::kStmtSelect)) {
    throw std::runtime_error(
        "ProjectionExecutor should construct with a select statement.");
  }
  child_executor_ = std::make_unique<SeqScanExecutor>(catalog, state);

  auto select = static_cast<const hsql::SelectStatement *>(state);
  table_info_ = catalog->GetTable(select->fromTable->getName());

  tuple_size_ = 0;
  for (auto &c : *select->selectList) {
    switch (c->type) {
      case hsql::ExprType::kExprStar: {
        for (auto col : table_info_.value_type_) {
          tuple_cloums_.push_back(col);
          tuple_size_ += col.GetSize();
        }
        break;
      }
      case hsql::ExprType::kExprColumnRef: {
        for (auto col : table_info_.value_type_) {
          if (col.cloum_name_ == c->getName()) {
            tuple_cloums_.push_back(col);
            tuple_size_ += col.GetSize();
            break;
          }
        }
        break;
      }
      case hsql::ExprType::kExprLiteralInt: {
        CloumAtr atr{CloumType::INT, sizeof(4)};
        Cloum clo("unname_" + std::to_string(unname_var_.size()), atr);
        tuple_cloums_.push_back(clo);
        tuple_size_ += clo.GetSize();
        unname_var_.push_back(c->ival);
        break;
      }
      default: {
        throw std::runtime_error("only support char&int now.");
        break;
      }
    }
  }
}

auto ProjectionExecutor::GetOutputCols() -> std::vector<Cloum> {
  return tuple_cloums_;
}

ProjectionExecutor::~ProjectionExecutor() {}

auto ProjectionExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  auto child_output = child_executor_->GetOutputCols();
  Tuple tuple_from_table{table_info_.value_type_};
  RID rid_from_table{};
  if (child_executor_->Next(&tuple_from_table, &rid_from_table)) {
    std::deque<int> unname_var(unname_var_);
    auto src = (char *)malloc(tuple_size_);
    size_t offset = 0;
    for (auto col : tuple_cloums_) {
      if (strncmp(col.cloum_name_.c_str(), "unname_", 7) == 0) {
        switch (col.GetType()) {
          case CloumType::INT:
            memcpy(src + offset, &unname_var.front(), sizeof(int));
            unname_var.pop_front();
            offset += sizeof(int);
            break;

          default:
            throw std::runtime_error("unname var should only be int.");
            break;
        }
      } else {
        int index = GetIndexByName(col.cloum_name_, child_output);
        switch (col.GetType()) {
          case CloumType::INT:
            memcpy(src + offset, tuple_from_table.GetValueAt(index),
                   sizeof(int));
            offset += sizeof(int);
            break;
          case CloumType::CHAR:
            memcpy(src + offset, tuple_from_table.GetValueAt(index),
                   child_output[index].GetSize());
            offset += child_output[index].GetSize();
            break;

          default:
            throw std::runtime_error("unname var should only be int.");
            break;
        }
      }
    }
    tuple->SetValues(src);
    free(src);
    return true;
  }
  return false;
}
}  // namespace spdb