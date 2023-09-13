#include <iostream>
#include <string>
#include <vector>

#include "disk/disk_manager.h"
int main() {
  std::string name = "test.db";
  spdb::DiskManager disk(name);
  for (int i = 0; i < 5; i++) {
    auto id = static_cast<page_id_t>(i);
    char s[4096];
    std::cin >> s;
    disk.WritePage(id, s);
  }
  std::vector<page_id_t> keys{0, 4, 1, 3, 2};
  for (auto id : keys) {
    char s[4096];
    disk.ReadPage(id, s);
    std::cout << s << ' ';
    std::cout.flush();
  }
  return 0;
}