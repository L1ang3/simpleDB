#include "executor/projection_executor.h"

namespace spdb {
ProjectionExecutor::ProjectionExecutor(
    Catalog *catalog, const hsql::SQLStatement *state,
    std::unique_ptr<AbstractExecutor> child_executor)
    : AbstractExecutor(catalog), child_executor_(std::move(child_executor)) {
  if (!state->isType(hsql::StatementType::kStmtSelect)) {
    throw std::runtime_error(
        "ProjectionExecutor should construct with a select statement.");
  }
  auto select = static_cast<const hsql::SelectStatement *>(state);
  table_info_ = catalog->GetTable(select->fromTable->getName());
  for (auto &c : *select->selectList) {
    switch (c->type) {
      case hsql::ExprType::kExprStar:
        /* code */
        break;

      default:
        break;
    }
  }
}

ProjectionExecutor::~ProjectionExecutor() {}

auto ProjectionExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  Tuple tuple_from_table{table_info_.value_type_};
  RID rid_from_table{};
  while (child_executor_->Next(&tuple_from_table, &rid_from_table)) {
    // for (auto &c : *select->selectList) {
    //   switch (c->type) {
    //     case hsql::ExprType::kExprStar:
    //       /* code */
    //       break;

    //     default:
    //       break;
    //   }
    // }
  }
  return true;
}
}  // namespace spdb