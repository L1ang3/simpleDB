#include "executor/seq_scan_executor.h"

namespace spdb {
SeqScanExecutor::SeqScanExecutor(Catalog *catalog,
                                 const hsql::SQLStatement *state)
    : AbstractExecutor(catalog) {
  if (!state->isType(hsql::StatementType::kStmtSelect)) {
    throw std::runtime_error(
        "SeqScanExecutor should construct with a selectstatement.");
  }
  auto select = static_cast<const hsql::SelectStatement *>(state);
  if (!catalog->IsExisted(select->fromTable->getName())) {
    throw std::runtime_error("table is not existed.");
  }
  table_info_ = catalog->GetTable(select->fromTable->getName());
  disk_ = new DiskManager(table_info_.disk_name_);
  bpm_ = new BufferPoolManager(BUFFER_POOL_SIZE, disk_);
  BPlusTree table(bpm_, table_info_.key_type_, table_info_.value_type_,
                  table_info_.leaf_max_size_, table_info_.internal_max_size_,
                  table_info_.root_id_);
  table_iterator_ = table.Begin();
}

SeqScanExecutor::~SeqScanExecutor() {
  delete disk_;
  delete bpm_;
}

auto SeqScanExecutor::GetOutputCols() -> std::vector<Cloum> {
  return table_info_.value_type_;
}

auto SeqScanExecutor::Next(Tuple *tuple, RID *rid) -> bool {
  if (table_iterator_.IsEnd()) {
    return false;
  }
  auto [key, value] = *table_iterator_;

  *tuple = value;
  *rid = tuple->GetRid();
  ++table_iterator_;

  return true;
}
}  // namespace spdb
