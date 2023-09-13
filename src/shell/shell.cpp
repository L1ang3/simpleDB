#include <iostream>
#include <string>
#include <vector>

#include "buffer/buffer_pool_manager.h"
using namespace spdb;
using namespace std;

int main() {
  string db_name = "test.db";
  DiskManager disk(db_name);
  BufferPoolManager bpm(20, &disk);
}