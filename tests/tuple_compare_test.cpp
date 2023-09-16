#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "config/config.h"
#include "disk/tuple.h"
#include "gtest/gtest.h"
using namespace std;

namespace spdb {
TEST(TupleCompareTest, IntCompareTest) {
  srand(time(0));
  CloumAtr atr{CloumType::INT, 4};

  const size_t size = 4;
  vector<Cloum> cols;
  cols.reserve(size);
  auto src = (char*)malloc(size * 4);
  size_t offset = 0;
  vector<int> v1, v2;
  v1.reserve(size);
  v2.reserve(size);

  for (size_t i = 0; i < size; ++i) {
    string str = "";
    Cloum c{str, atr};
    cols.emplace_back(c);
    int tmp = random() % 2;
    v1.emplace_back(tmp);
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
    v2.emplace_back(tmp);
    memcpy(src + offset, &tmp, 4);
    offset += 4;
  }
  Tuple t2(cols);
  t2.SetValues(src);

  ASSERT_EQ(v1 < v2, t1 < t2);
  ASSERT_EQ(v1 > v2, t1 > t2);
  ASSERT_EQ(v1 == v2, t1 == t2);
  free(src);
}
}  // namespace spdb