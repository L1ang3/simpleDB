#include "buffer/lru_k_replacer.h"

namespace spdb {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k)
    : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);

  bool ret = false;
  for (auto [fid, lrunode] : node_store_) {
    if (lrunode.IsEvictable()) {
      if (!ret) {
        *frame_id = fid;
        ret = true;
        continue;
      }
      if (lrunode.GetKdistance(current_timestamp_) >
              node_store_[*frame_id].GetKdistance(current_timestamp_) ||
          (lrunode.GetKdistance(current_timestamp_) ==
               node_store_[*frame_id].GetKdistance(current_timestamp_) &&
           lrunode.GetFirstTimeStamp() <
               node_store_[*frame_id].GetFirstTimeStamp())) {
        *frame_id = fid;
      }
    }
  }
  if (ret) {
    node_store_.erase(*frame_id);
    curr_size_--;
  }

  return ret;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id,
                                [[maybe_unused]] AccessType access_type) {
  std::lock_guard<std::mutex> lock(latch_);

  current_timestamp_++;
  if (node_store_.find(frame_id) != node_store_.end()) {
    node_store_[frame_id].Access(current_timestamp_);
  } else {
    if (node_store_.size() < replacer_size_) {
      node_store_[frame_id] = LRUKNode(frame_id, k_, current_timestamp_);
    } else {
      throw std::runtime_error("RecordAccess: this replacer is already full");
    }
  }
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::lock_guard<std::mutex> lock(latch_);
  if (node_store_.find(frame_id) != node_store_.end()) {
    if (node_store_[frame_id].IsEvictable() != set_evictable) {
      if (set_evictable) {
        curr_size_++;
      } else {
        curr_size_--;
      }
      node_store_[frame_id].SetEvictable(set_evictable);
    }
  } else {
    throw std::runtime_error("SetEvictable: this frame is not existed");
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::lock_guard<std::mutex> lock(latch_);
  if (node_store_.find(frame_id) != node_store_.end()) {
    if (node_store_[frame_id].IsEvictable()) {
      curr_size_--;
      node_store_.erase(frame_id);
    } else {
      throw std::runtime_error("Remove: removing a non-evictable frame");
    }
  }
}

auto LRUKReplacer::Size() -> size_t {
  std::lock_guard<std::mutex> lock(latch_);
  size_t size;
  size = curr_size_;
  return size;
}

auto LRUKReplacer::FrameSize() -> size_t {
  std::lock_guard<std::mutex> lock(latch_);
  size_t size;
  size = node_store_.size();
  return size;
}

auto LRUKReplacer::IsExisted(frame_id_t frame_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  bool ret = false;

  for (auto &it : node_store_) {
    if (it.first == frame_id) {
      ret = true;
      break;
    }
  }

  return ret;
}

auto LRUKReplacer::IsEvictable(frame_id_t frame_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  bool ret = false;
  ret = node_store_[frame_id].IsEvictable();
  return ret;
}

}  // namespace spdb
