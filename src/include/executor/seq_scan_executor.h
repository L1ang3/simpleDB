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
  SeqScanExecutor(Catalog *catalog, hsql::SQLStatement *);

  ~SeqScanExecutor();

  auto Next(Tuple *tuple, RID *rid) -> bool override;

 private:
  Iterator table_iterator_;
  DiskManager *disk_;
  BufferPoolManager *bpm_;
};
}  // namespace spdb