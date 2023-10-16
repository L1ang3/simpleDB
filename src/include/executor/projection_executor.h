#pragma once

#include <deque>
#include <memory>
#include <vector>

#include "abstract_executor.h"
#include "seq_scan_executor.h"

namespace spdb {
class ProjectionExecutor : public AbstractExecutor {
 public:
  ProjectionExecutor(Catalog *catalog, const hsql::SQLStatement *);

  ~ProjectionExecutor();

  auto Next(Tuple *tuple, RID *rid) -> bool override;

  auto GetOutputCols() -> std::vector<Cloum> override;

 private:
  std::unique_ptr<AbstractExecutor> child_executor_;
  std::vector<Cloum> tuple_cloums_;
  size_t tuple_size_;
  std::deque<int> unname_var_;
  TableInfo table_info_;
};
}  // namespace spdb
