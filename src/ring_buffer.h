#pragma once

#include <atomic>
#include <vector>

// SPSC ring buffer - decode thread writes, SDL callback reads
class RingBuffer {
public:
    explicit RingBuffer(size_t cap)
        : buffer_(cap + 1), capacity_(cap + 1), head_(0), tail_(0) {}

    // write samples, drop if full (non-blocking)
    size_t Write(const float* data, size_t count)
    {
        size_t written = 0;
        size_t h = head_.load(std::memory_order_relaxed);
        size_t t = tail_.load(std::memory_order_acquire);

        for (size_t i = 0; i < count; i++) {
            size_t next = (h + 1) % capacity_;
            if (next == t) {
                // full, drop rest
                break;
            }
            buffer_[h] = data[i];
            h = next;
            written++;
        }

        head_.store(h, std::memory_order_release);
        return written;
    }

    size_t Read(float* out, size_t count)
    {
        size_t n = 0;
        size_t t = tail_.load(std::memory_order_relaxed);
        size_t h = head_.load(std::memory_order_acquire);

        while (n < count && t != h) {
            out[n++] = buffer_[t];
            t = (t + 1) % capacity_;
        }

        tail_.store(t, std::memory_order_release);
        return n;
    }

    size_t Available() const
    {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return (h >= t) ? (h - t) : (capacity_ - t + h);
    }

    void Clear() {
        tail_.store(head_.load(std::memory_order_acquire), std::memory_order_release);
    }

private:
    std::vector<float> buffer_;
    size_t capacity_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};
