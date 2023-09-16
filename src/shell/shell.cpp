#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "buffer/buffer_pool_manager.h"
#include "config/config.h"
#include "disk/tuple.h"
using namespace spdb;
using namespace std;

int main() {
  srand(time(0));
  CloumAtr atr{CloumType::INT, 4};
  const size_t size = 2;
  vector<Cloum> cols;
  cols.reserve(size);
  auto src = (char*)malloc(size * 4);
  size_t offset = 0;
  for (size_t i = 0; i < size; ++i) {
    string str = "";
    Cloum c{str, atr};
    cols.emplace_back(c);
    int tmp = random() % 2;
    memcpy(src + offset, &tmp, 4);
    offset += 4;
  }
  Tuple t1(cols);
  t1.SetValues(src);

  offset = 0;
  for (size_t i = 0; i < size; ++i) {
    string str = "";
    Cloum c{str, atr};
    cols.emplace_back(c);
    int tmp = random() % 2;
    memcpy(src + offset, &tmp, 4);
    offset += 4;
  }
  Tuple t2(cols);
  t2.SetValues(src);

  cout << "t1 t2\n";
  for (size_t i = 0; i < size; ++i) {
    cout << *t1.GetValueAtAs<int>(i) << ' ' << *t2.GetValueAtAs<int>(i) << '\n';
  }
  if (t1 < t2) {
    cout << "t1<t2\n";
  } else if (t1 > t2) {
    cout << "t1>t2\n";
  } else if (t1 == t2) {
    cout << "t1==t2\n";
  }
}