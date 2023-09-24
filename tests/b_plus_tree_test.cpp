#include "table/b_plus_tree.h"

#include <thread>

#include "gtest/gtest.h"

namespace spdb {
template <typename... Args>
void LaunchParallelTest(uint64_t num_threads, Args &&...args) {
  std::vector<std::thread> thread_group;

  // Launch a group of threads
  for (uint64_t thread_itr = 0; thread_itr < num_threads; ++thread_itr) {
    thread_group.push_back(std::thread(args..., thread_itr));
  }

  // Join the threads with the main thread
  for (uint64_t thread_itr = 0; thread_itr < num_threads; ++thread_itr) {
    thread_group[thread_itr].join();
  }
}

// helper function to insert
void InsertHelper(BPlusTree *tree, const std::vector<int32_t> &keys,
                  __attribute__((unused)) uint64_t thread_itr = 0) {
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  for (auto key : keys) {
    index_key.SetValues((char *)(&key));
    tree->Insert(index_key, index_key);
  }
}

// helper function to seperate insert
void InsertHelperSplit(BPlusTree *tree, const std::vector<int32_t> &keys,
                       int total_threads,
                       __attribute__((unused)) uint64_t thread_itr) {
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  for (auto key : keys) {
    if (static_cast<uint64_t>(key) % total_threads == thread_itr) {
      index_key.SetValues((char *)(&key));
      tree->Insert(index_key, index_key);
    }
  }
}

// helper function to delete
void DeleteHelper(BPlusTree *tree, const std::vector<int32_t> &remove_keys,
                  __attribute__((unused)) uint64_t thread_itr = 0) {
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  for (auto key : remove_keys) {
    index_key.SetValues((char *)(&key));
    tree->Remove(index_key);
  }
}

// helper function to seperate delete
void DeleteHelperSplit(BPlusTree *tree, const std::vector<int32_t> &remove_keys,
                       int total_threads,
                       __attribute__((unused)) uint64_t thread_itr) {
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  for (auto key : remove_keys) {
    if (static_cast<uint64_t>(key) % total_threads == thread_itr) {
      index_key.SetValues((char *)(&key));
      tree->Remove(index_key);
    }
  }
}

void LookupHelper(BPlusTree *tree, const std::vector<int32_t> &keys,
                  uint64_t tid,
                  __attribute__((unused)) uint64_t thread_itr = 0) {
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  for (auto key : keys) {
    Tuple result(type);
    index_key.SetValues((char *)(&key));
    bool res = tree->GetValue(index_key, result);
    ASSERT_EQ(res, true);
    ASSERT_EQ(*result.GetValueAtAs<int32_t>(0), key);
  }
}

TEST(BPlusTreeConcurrentTest, InsertTest1) {
  auto disk = DiskManager("b_plus_tree_test_disk");
  auto *bpm = new BufferPoolManager(50, &disk);
  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(&page_id);
  auto tmp = sizeof(size_t);
  // create b+ tree
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  BPlusTree tree(header_page->GetPageId(), bpm, type, type, 2, 3);
  // keys to Insert
  std::vector<int32_t> keys;
  int64_t scale_factor = 300;
  for (int64_t key = 1; key < scale_factor; key++) {
    keys.push_back(key);
  }
  LaunchParallelTest(10, InsertHelper, &tree, keys);
  for (auto key : keys) {
    Tuple result(type);

    index_key.SetValues((char *)(&key));
    tree.GetValue(index_key, result);
    EXPECT_EQ(*result.GetValueAtAs<int32_t>(0), key);
  }

  int32_t start_key = 1;
  int32_t current_key = start_key;
  index_key.SetValues((char *)&start_key);
  for (auto iterator = tree.Begin(index_key); iterator != tree.End();
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(*location.GetValueAtAs<int32_t>(0), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(current_key, keys.size() + 1);

  bpm->UnpinPage(page_id, true);
  delete bpm;
}

TEST(BPlusTreeConcurrentTest, InsertTest2) {
  auto disk = DiskManager("b_plus_tree_test_disk");
  auto *bpm = new BufferPoolManager(50, &disk);
  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(&page_id);
  // create b+ tree
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  int leaf_max_size = (PAGE_SIZE - LEAF_HEADER_SIZE) / (type[0].GetSize() * 2);
  int internal_max_size =
      (PAGE_SIZE - INTERNAL_HEADER_SIZE) / (type[0].GetSize() * 2);
  BPlusTree tree(header_page->GetPageId(), bpm, type, type, leaf_max_size,
                 internal_max_size);
  // keys to Insert
  std::vector<int32_t> keys;
  int32_t scale_factor = 10000;
  for (int32_t key = 1; key < scale_factor; key++) {
    keys.push_back(key);
  }
  LaunchParallelTest(6, InsertHelperSplit, &tree, keys, 2);

  for (auto key : keys) {
    index_key.SetValues((char *)&key);
    Tuple result(type);
    tree.GetValue(index_key, result);
    EXPECT_EQ(*result.GetValueAtAs<int32_t>(0), key);
  }

  int32_t start_key = 1;
  int32_t current_key = start_key;
  index_key.SetValues((char *)&start_key);
  for (auto iterator = tree.Begin(index_key); iterator != tree.End();
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(*location.GetValueAtAs<int32_t>(0), current_key);
    current_key = current_key + 1;
  }

  EXPECT_EQ(current_key, keys.size() + 1);

  bpm->UnpinPage(page_id, true);
  delete bpm;
}

TEST(BPlusTreeConcurrentTest, DeleteTest1) {
  // create KeyComparator and index schema
  auto disk = DiskManager("b_plus_tree_test_disk");
  auto *bpm = new BufferPoolManager(50, &disk);
  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(&page_id);
  // create b+ tree
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  int leaf_max_size = (PAGE_SIZE - LEAF_HEADER_SIZE) / (type[0].GetSize() * 2);
  int internal_max_size =
      (PAGE_SIZE - INTERNAL_HEADER_SIZE) / (type[0].GetSize() * 2);
  BPlusTree tree(header_page->GetPageId(), bpm, type, type, leaf_max_size,
                 internal_max_size);
  // sequential insert
  std::vector<int32_t> keys = {1, 2, 3, 4, 5};
  InsertHelper(&tree, keys);
  std::vector<int32_t> remove_keys = {1, 5, 3, 4};
  LaunchParallelTest(2, DeleteHelper, &tree, remove_keys);
  int32_t start_key = 2;
  int32_t current_key = start_key;
  int32_t size = 0;
  index_key.SetValues((char *)&start_key);
  for (auto iterator = tree.Begin(index_key); iterator != tree.End();
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(*location.GetValueAtAs<int32_t>(0), current_key);
    current_key = current_key + 1;
    size = size + 1;
  }

  EXPECT_EQ(size, 1);

  bpm->UnpinPage(page_id, true);
  delete bpm;
}

TEST(BPlusTreeConcurrentTest, DeleteTest2) {
  auto disk = DiskManager("b_plus_tree_test_disk");
  auto *bpm = new BufferPoolManager(50, &disk);
  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(&page_id);
  // create b+ tree
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  int leaf_max_size = (PAGE_SIZE - LEAF_HEADER_SIZE) / (type[0].GetSize() * 2);
  int internal_max_size =
      (PAGE_SIZE - INTERNAL_HEADER_SIZE) / (type[0].GetSize() * 2);
  BPlusTree tree(header_page->GetPageId(), bpm, type, type, leaf_max_size,
                 internal_max_size);
  // sequential insert
  std::vector<int32_t> keys = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  InsertHelper(&tree, keys);

  std::vector<int32_t> remove_keys = {1, 4, 3, 2, 5, 6};
  LaunchParallelTest(2, DeleteHelperSplit, &tree, remove_keys, 2);

  int64_t start_key = 7;
  int64_t current_key = start_key;
  int64_t size = 0;
  index_key.SetValues((char *)&start_key);
  for (auto iterator = tree.Begin(index_key); iterator != tree.End();
       ++iterator) {
    auto location = (*iterator).second;
    EXPECT_EQ(*location.GetValueAtAs<int32_t>(0), current_key);
    current_key = current_key + 1;
    size = size + 1;
  }

  EXPECT_EQ(size, 4);

  bpm->UnpinPage(page_id, true);
  delete bpm;
}

TEST(BPlusTreeConcurrentTest, MixTest1) {
  auto disk = DiskManager("b_plus_tree_test_disk");
  auto *bpm = new BufferPoolManager(50, &disk);
  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(&page_id);
  // create b+ tree
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  int leaf_max_size = (PAGE_SIZE - LEAF_HEADER_SIZE) / (type[0].GetSize() * 2);
  int internal_max_size =
      (PAGE_SIZE - INTERNAL_HEADER_SIZE) / (type[0].GetSize() * 2);
  BPlusTree tree(header_page->GetPageId(), bpm, type, type, leaf_max_size,
                 internal_max_size);
  // first, populate index
  std::vector<int32_t> keys = {1, 2, 3, 4, 5};
  InsertHelper(&tree, keys);

  // concurrent insert
  keys.clear();
  for (int i = 6; i <= 10; i++) {
    keys.push_back(i);
  }
  LaunchParallelTest(1, InsertHelper, &tree, keys);
  // concurrent delete
  std::vector<int32_t> remove_keys = {1, 4, 3, 5, 6};
  LaunchParallelTest(1, DeleteHelper, &tree, remove_keys);

  int32_t start_key = 2;
  int32_t size = 0;
  index_key.SetValues((char *)&start_key);
  for (auto iterator = tree.Begin(index_key); iterator != tree.End();
       ++iterator) {
    size = size + 1;
  }

  EXPECT_EQ(size, 5);

  bpm->UnpinPage(page_id, true);
  delete bpm;
}

TEST(BPlusTreeConcurrentTest, MixTest2) {
  auto disk = DiskManager("b_plus_tree_test_disk");
  auto *bpm = new BufferPoolManager(50, &disk);
  // create and fetch header_page
  page_id_t page_id;
  auto header_page = bpm->NewPage(&page_id);
  // create b+ tree
  Cloum c{"value", {CloumType::INT, 4}};
  std::vector<Cloum> type{c};
  Tuple index_key(type);
  int leaf_max_size = (PAGE_SIZE - LEAF_HEADER_SIZE) / (type[0].GetSize() * 2);
  int internal_max_size =
      (PAGE_SIZE - INTERNAL_HEADER_SIZE) / (type[0].GetSize() * 2);
  BPlusTree tree(header_page->GetPageId(), bpm, type, type, leaf_max_size,
                 internal_max_size);

  // Add perserved_keys
  std::vector<int32_t> perserved_keys;
  std::vector<int32_t> dynamic_keys;
  int32_t total_keys = 1000;
  int32_t sieve = 5;
  for (int32_t i = 1; i <= total_keys; i++) {
    if (i % sieve == 0) {
      perserved_keys.push_back(i);
    } else {
      dynamic_keys.push_back(i);
    }
  }
  InsertHelper(&tree, perserved_keys, 1);
  // Check there are 1000 keys in there
  size_t size;

  auto insert_task = [&](int tid) { InsertHelper(&tree, dynamic_keys, tid); };
  auto delete_task = [&](int tid) { DeleteHelper(&tree, dynamic_keys, tid); };
  auto lookup_task = [&](int tid) { LookupHelper(&tree, perserved_keys, tid); };

  std::vector<std::thread> threads;
  std::vector<std::function<void(int)>> tasks;
  tasks.emplace_back(insert_task);
  tasks.emplace_back(delete_task);
  tasks.emplace_back(lookup_task);

  size_t num_threads = 60;
  for (size_t i = 0; i < num_threads; i++) {
    threads.emplace_back(std::thread{tasks[i % tasks.size()], i});
  }
  for (size_t i = 0; i < num_threads; i++) {
    threads[i].join();
  }
  // Check all reserved keys exist
  size = 0;

  for (auto iter = tree.Begin(); iter != tree.End(); ++iter) {
    const auto &pair = *iter;
    if (*(pair.first).GetValueAtAs<int32_t>(0) % sieve == 0) {
      size++;
    }
  }

  ASSERT_EQ(size, perserved_keys.size());

  bpm->UnpinPage(page_id, true);
  delete bpm;
}

}  // namespace spdb
