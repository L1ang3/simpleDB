#pragma once

#include <queue>

#include "abstract_executor.h"
namespace spdb {
class ValueExecutor : public AbstractExecutor {
 public:
  ValueExecutor(Catalog *catalog, const hsql::SQLStatement *);

  ~ValueExecutor();

  auto Next(Tuple *tuple, RID *rid) -> bool override;

  auto GetOutputCols() -> std::vector<Cloum> override;

 private:
  TableInfo table_info_;
  std::queue<Tuple> values_;
};
}  // namespace spdb
