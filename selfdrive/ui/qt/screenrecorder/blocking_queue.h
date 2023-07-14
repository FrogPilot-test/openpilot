#pragma once
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>

template <class T>
class BlockingQueue {
public:
  BlockingQueue(size_t capacity): capacity(capacity) {}

  BlockingQueue(const BlockingQueue &) = delete;
  BlockingQueue(BlockingQueue &&) = delete;
  BlockingQueue &operator = (const BlockingQueue &) = delete;
  BlockingQueue &operator = (BlockingQueue &&) = delete;

  void clear() {
    std::unique_lock<std::mutex> lk(mutex);
    content.clear();
    not_full.notify_one();
  }

  void push(T &&item) {
    std::unique_lock<std::mutex> lk(mutex);
    not_full.wait(lk, [this]() { return content.size() < capacity; });
    content.push_back(std::move(item));
    not_empty.notify_one();
  }

  bool try_push(T &&item) {
    std::unique_lock<std::mutex> lk(mutex);
    if (content.size() == capacity) return false;
    content.push_back(std::move(item));
    not_empty.notify_one();
    return true;
  }

  void pop(T &item) {
    std::unique_lock<std::mutex> lk(mutex);
    not_empty.wait(lk, [this]() { return content.size(); });
    item = std::move(content.front());
    content.pop_front();
    not_full.notify_one();
  }

  bool pop_wait_for(T &item, std::chrono::milliseconds duration) {
    std::unique_lock<std::mutex> lk(mutex);
    if (!not_empty.wait_for(lk, duration, [this]() { return content.size(); })) return false;
    item = std::move(content.front());
    content.pop_front();
    not_full.notify_one();
    return true;
  }

  bool try_pop(T &item) {
    std::unique_lock<std::mutex> lk(mutex);
    if (content.empty()) return false;
    item = std::move(content.front());
    content.pop_front();
    not_full.notify_one();
    return true;
  }

private:
  std::deque<T> content;
  size_t capacity;
  std::mutex mutex;
  std::condition_variable not_empty;
  std::condition_variable not_full;
};
