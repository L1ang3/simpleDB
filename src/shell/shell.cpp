#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "config/config.h"
#include "disk/tuple.h"
#include "table/b_plus_tree.h"
using namespace spdb;
using namespace std;

int main() {
  DiskManager disk("test_disk");
  BufferPoolManager bpm(20, &disk);
  page_id_t pid = INVALID_PAGE_ID;
  bpm.NewPageGuarded(&pid);
  Cloum v("v1", {CloumType::INT, 4});
  vector<Cloum> key_type{v};
  vector<Cloum> value_type{v};
  BPlusTree bpt("test.db", pid, &bpm, key_type, value_type, 2, 3);

  Tuple key(key_type);
  Tuple value(value_type);
  vector<int> keys{1, 2, 3, 4, 5};
  for (auto i : keys) {
    key.SetValues((char*)(&i));
    value.SetValues((char*)(&i));
    bpt.Insert(key, value);
  }
}