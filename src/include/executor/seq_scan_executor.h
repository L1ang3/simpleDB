#pragma once

#include <memory>
#include <vector>

#include "abstract_executor.h"
namespace spdb {

/**
 * The SeqScanExecutor executor executes a sequential table scan.
 */
class SeqScanExecutor : public AbstractExecutor {
 public:
  SeqScanExecutor(Catalog *catalog, const hsql::SQLStatement *);

  ~SeqScanExecutor();

  auto Next(Tuple *tuple, RID *rid) -> bool override;

  auto GetOutputCols() -> std::vector<Cloum> override;

 private:
  TableInfo table_info_;
  Iterator table_iterator_;
  DiskManager *disk_;
  BufferPoolManager *bpm_;
};
}  // namespace spdb