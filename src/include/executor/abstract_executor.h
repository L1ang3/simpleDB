#pragma once

#include "SQLParser.h"
#include "config/catalog.h"
#include "config/config.h"
#include "table/b_plus_tree.h"

namespace spdb {
class AbstractExecutor {
 public:
  AbstractExecutor(Catalog *catalog) : catalog_(catalog) {}

  virtual auto Next(Tuple *tuple, RID *rid) -> bool = 0;

 private:
  Catalog *catalog_;
};
}  // namespace spdb
