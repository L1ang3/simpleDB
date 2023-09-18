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

  const size_t size = 8;
  vector<Cloum> cols;
  cols.reserve(size);
  auto src = (char *)malloc(size * 4);
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

  cols.clear();
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

  auto si = sizeof(src);
  ASSERT_EQ(v1 < v2, t1 < t2);
  ASSERT_EQ(v1 > v2, t1 > t2);
  ASSERT_EQ(v1 == v2, t1 == t2);
  free(src);
}

TEST(TupleCompareTest, StringCompareTest) {
  srand(time(0));
  string str = "";
  CloumAtr ca1{CloumType::STRING, 7};
  CloumAtr ca2{CloumType::STRING, 4};
  CloumAtr ca3{CloumType::STRING, 6};
  Cloum c1{str, ca1};
  Cloum c2{str, ca1};
  Cloum c3{str, ca1};
  vector<Cloum> cols;
  cols.push_back(c1);
  cols.push_back(c2);
  cols.push_back(c3);

  Tuple t1(cols), t2(cols);
  char s[7] = "12ad59";
  char v2s1[4] = "321";
  char v2s2[4] = "322";
  char v3s[6] = "12345";

  auto src = (char *)malloc(17);
  memcpy(src, s, 7);
  memcpy(src + 7, v2s1, 4);
  memcpy(src + 11, v3s, 6);
  t1.SetValues(src);

  memcpy(src, s, 7);
  memcpy(src + 7, v2s2, 4);
  memcpy(src + 11, v3s, 6);
  t2.SetValues(src);

  ASSERT_EQ(true, t1 < t2);
  ASSERT_EQ(false, t1 > t2);
  ASSERT_EQ(false, t1 == t2);
  free(src);
}
}  // namespace spdb