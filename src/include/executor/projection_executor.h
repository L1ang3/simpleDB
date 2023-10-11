#pragma once

#include <memory>
#include <vector>

#include "abstract_executor.h"

namespace spdb {
class ProjectionExecutor : public AbstractExecutor {
 public:
  ProjectionExecutor(Catalog *catalog, const hsql::SQLStatement *,
                     std::unique_ptr<AbstractExecutor> child_executor);

  ~ProjectionExecutor();

  auto Next(Tuple *tuple, RID *rid) -> bool override;

 private:
  std::unique_ptr<AbstractExecutor> child_executor_;
  std::vector<size_t> tuple_indexs_;
  TableInfo table_info_;
};
}  // namespace spdb
