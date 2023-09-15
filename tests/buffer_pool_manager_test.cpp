#include "buffer/buffer_pool_manager.h"

#include <cstdio>
#include <cstring>
#include <random>
#include <string>

#include "config/config.h"
#include "disk/disk_manager.h"
#include "gtest/gtest.h"

namespace spdb {
TEST(LRUKReplacerTest, SampleTest) {
  LRUKReplacer lru_replacer(7, 2);

  // Scenario: add six elements to the replacer. We have [1,2,3,4,5]. Frame 6 is
  // non-evictable.
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(2);
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(5);
  lru_replacer.RecordAccess(6);
  lru_replacer.SetEvictable(1, true);
  lru_replacer.SetEvictable(2, true);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);
  lru_replacer.SetEvictable(5, true);
  lru_replacer.SetEvictable(6, false);
  ASSERT_EQ(lru_replacer.Size(), 5);

  // Scenario: Insert access history for frame 1. Now frame 1 has two access
  // histories. All other frames have max backward k-dist. The order of eviction
  // is [2,3,4,5,1].
  lru_replacer.RecordAccess(1);

  // Scenario: Evict three pages from the replacer. Elements with max k-distance
  // should be popped first based on LRU.
  frame_id_t value;
  lru_replacer.Evict(&value);
  ASSERT_EQ(2, value);
  lru_replacer.Evict(&value);
  ASSERT_EQ(3, value);
  lru_replacer.Evict(&value);
  ASSERT_EQ(4, value);
  ASSERT_EQ(2, lru_replacer.Size());

  // Scenario: Now replacer has frames [5,1].
  // Insert new frames 3, 4, and update access history for 5. We should end with
  // [3,1,5,4]
  lru_replacer.RecordAccess(3);
  lru_replacer.RecordAccess(4);
  lru_replacer.RecordAccess(5);
  lru_replacer.RecordAccess(4);
  lru_replacer.SetEvictable(3, true);
  lru_replacer.SetEvictable(4, true);
  ASSERT_EQ(4, lru_replacer.Size());

  // Scenario: continue looking for victims. We expect 3 to be evicted next.
  lru_replacer.Evict(&value);
  ASSERT_EQ(3, value);
  ASSERT_EQ(3, lru_replacer.Size());

  // Set 6 to be evictable. 6 Should be evicted next since it has max backward
  // k-dist.
  lru_replacer.SetEvictable(6, true);
  ASSERT_EQ(4, lru_replacer.Size());
  lru_replacer.Evict(&value);
  ASSERT_EQ(6, value);
  ASSERT_EQ(3, lru_replacer.Size());

  // Now we have [1,5,4]. Continue looking for victims.
  lru_replacer.SetEvictable(1, false);
  ASSERT_EQ(2, lru_replacer.Size());
  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(5, value);
  ASSERT_EQ(1, lru_replacer.Size());

  // Update access history for 1. Now we have [4,1]. Next victim is 4.
  lru_replacer.RecordAccess(1);
  lru_replacer.RecordAccess(1);
  lru_replacer.SetEvictable(1, true);
  ASSERT_EQ(2, lru_replacer.Size());
  ASSERT_EQ(true, lru_replacer.Evict(&value));
  ASSERT_EQ(value, 4);

  ASSERT_EQ(1, lru_replacer.Size());
  lru_replacer.Evict(&value);
  ASSERT_EQ(value, 1);
  ASSERT_EQ(0, lru_replacer.Size());

  // This operation should not modify size
  ASSERT_EQ(false, lru_replacer.Evict(&value));
  ASSERT_EQ(0, lru_replacer.Size());
}

// NOLINTNEXTLINE
// Check whether pages containing terminal characters can be recovered
TEST(BufferPoolManagerTest, BinaryDataTest) {
  const std::string db_name = "test.db";
  const size_t buffer_pool_size = 10;
  const size_t k = 5;

  std::random_device r;
  std::default_random_engine rng(r());
  std::uniform_int_distribution<char> uniform_dist(0);

  auto *disk_manager = new DiskManager(db_name);
  auto *bpm = new BufferPoolManager(buffer_pool_size, disk_manager, k);

  page_id_t page_id_temp;
  auto *page0 = bpm->NewPage(&page_id_temp);

  // Scenario: The buffer pool is empty. We should be able to create a new page.
  ASSERT_NE(nullptr, page0);
  EXPECT_EQ(0, page_id_temp);

  char random_binary_data[PAGE_SIZE];
  // Generate random binary data
  for (char &i : random_binary_data) {
    i = uniform_dist(rng);
  }

  // Insert terminal characters both in the middle and at end
  random_binary_data[PAGE_SIZE / 2] = '\0';
  random_binary_data[PAGE_SIZE - 1] = '\0';

  // Scenario: Once we have a page, we should be able to read and write content.
  std::memcpy(page0->GetData(), random_binary_data, PAGE_SIZE);
  EXPECT_EQ(0, std::memcmp(page0->GetData(), random_binary_data, PAGE_SIZE));

  // Scenario: We should be able to create new pages until we fill up the buffer
  // pool.
  for (size_t i = 1; i < buffer_pool_size; ++i) {
    EXPECT_NE(nullptr, bpm->NewPage(&page_id_temp));
  }

  // Scenario: Once the buffer pool is full, we should not be able to create any
  // new pages.
  for (size_t i = buffer_pool_size; i < buffer_pool_size * 2; ++i) {
    EXPECT_EQ(nullptr, bpm->NewPage(&page_id_temp));
  }

  // Scenario: After unpinning pages {0, 1, 2, 3, 4} we should be able to create
  // 5 new pages
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(true, bpm->UnpinPage(i, true));
    bpm->FlushPage(i);
  }
  for (int i = 0; i < 5; ++i) {
    EXPECT_NE(nullptr, bpm->NewPage(&page_id_temp));
    bpm->UnpinPage(page_id_temp, false);
  }
  // Scenario: We should be able to fetch the data we wrote a while ago.
  page0 = bpm->FetchPage(0);
  EXPECT_EQ(0, memcmp(page0->GetData(), random_binary_data, PAGE_SIZE));
  EXPECT_EQ(true, bpm->UnpinPage(0, true));

  // Shutdown the disk manager and remove the temporary file we created.
  disk_manager->ShutDown();
  remove("test.db");

  delete bpm;
  delete disk_manager;
}

// NOLINTNEXTLINE
TEST(BufferPoolManagerTest, SampleTest) {
  const std::string db_name = "test.db";
  const size_t buffer_pool_size = 10;
  const size_t k = 5;

  auto *disk_manager = new DiskManager(db_name);
  auto *bpm = new BufferPoolManager(buffer_pool_size, disk_manager, k);

  page_id_t page_id_temp;
  auto *page0 = bpm->NewPage(&page_id_temp);

  // Scenario: The buffer pool is empty. We should be able to create a new page.
  ASSERT_NE(nullptr, page0);
  EXPECT_EQ(0, page_id_temp);

  // Scenario: Once we have a page, we should be able to read and write content.
  snprintf(page0->GetData(), PAGE_SIZE, "Hello");
  EXPECT_EQ(0, strcmp(page0->GetData(), "Hello"));

  // Scenario: We should be able to create new pages until we fill up the buffer
  // pool.
  for (size_t i = 1; i < buffer_pool_size; ++i) {
    EXPECT_NE(nullptr, bpm->NewPage(&page_id_temp));
  }

  // Scenario: Once the buffer pool is full, we should not be able to create any
  // new pages.
  for (size_t i = buffer_pool_size; i < buffer_pool_size * 2; ++i) {
    EXPECT_EQ(nullptr, bpm->NewPage(&page_id_temp));
  }

  // Scenario: After unpinning pages {0, 1, 2, 3, 4} and pinning another 4 new
  // pages, there would still be one buffer page left for reading page 0.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(true, bpm->UnpinPage(i, true));
  }
  for (int i = 0; i < 4; ++i) {
    EXPECT_NE(nullptr, bpm->NewPage(&page_id_temp));
  }

  // Scenario: We should be able to fetch the data we wrote a while ago.
  page0 = bpm->FetchPage(0);
  EXPECT_EQ(0, strcmp(page0->GetData(), "Hello"));

  // Scenario: If we unpin page 0 and then make a new page, all the buffer pages
  // should now be pinned. Fetching page 0 should fail.
  EXPECT_EQ(true, bpm->UnpinPage(0, true));
  EXPECT_NE(nullptr, bpm->NewPage(&page_id_temp));
  EXPECT_EQ(nullptr, bpm->FetchPage(0));

  // Shutdown the disk manager and remove the temporary file we created.
  disk_manager->ShutDown();
  remove("test.db");

  delete bpm;
  delete disk_manager;
}

}  // namespace spdb
