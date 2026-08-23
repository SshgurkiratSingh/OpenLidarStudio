#pragma once

#include <atomic>
#include <cstddef>
#include <vector>
#include <new>

#if defined(__cpp_lib_hardware_interference_size)
    constexpr std::size_t cache_line_size = std::hardware_destructive_interference_size;
#else
    constexpr std::size_t cache_line_size = 64;
#endif

namespace ols::core {

template <typename T>
class alignas(cache_line_size) RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity)
        : capacity_(capacity + 1), // One slot open to distinguish full vs empty
          buffer_(capacity_),
          head_(0),
          tail_(0) {}

    bool push(const T& item) {
        auto current_tail = tail_.load(std::memory_order_relaxed);
        auto next_tail = increment(current_tail);
        if (next_tail != head_.load(std::memory_order_acquire)) {
            buffer_[current_tail] = item;
            tail_.store(next_tail, std::memory_order_release);
            return true;
        }
        return false; // Full
    }

    bool pop(T& item) {
        auto current_head = head_.load(std::memory_order_relaxed);
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Empty
        }
        item = buffer_[current_head];
        head_.store(increment(current_head), std::memory_order_release);
        return true;
    }

private:
    std::size_t increment(std::size_t n) const {
        return (n + 1) % capacity_;
    }

    std::size_t capacity_;
    std::vector<T> buffer_;
    alignas(cache_line_size) std::atomic<std::size_t> head_;
    alignas(cache_line_size) std::atomic<std::size_t> tail_;
};

} // namespace ols::core
