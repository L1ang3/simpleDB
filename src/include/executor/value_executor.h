#pragma once

#include <vector>

#include "abstract_executor.h"
namespace spdb {
class ValueExecutor : public AbstractExecutor {
 public:
  ValueExecutor(Catalog *catalog, hsql::SQLStatement *);

  ~ValueExecutor();

  auto Next(Tuple *tuple, RID *rid) -> bool override;

 private:
  std::vector<Tuple> values_;
};
}  // namespace spdb
