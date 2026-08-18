#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>

/** Fixed-capacity FIFO ring buffer; storage inline, no heap. */
template <typename T, std::size_t N>
class RingBuffer {
    static_assert(N > 0 && N <= 256, "capacity must be 1..256");

public:
    inline void push(const T& v) {
        assert(size_ < N);
        buf_[static_cast<std::uint8_t>((head_ + size_) % N)] = v;
        ++size_;
    }

    inline void pop() {
        assert(size_ > 0);
        head_ = static_cast<std::uint8_t>((head_ + 1u) % N);
        --size_;
    }

    inline T& front() {
        assert(size_ > 0);
        return buf_[head_];
    }

    inline const T& front() const {
        assert(size_ > 0);
        return buf_[head_];
    }

    inline bool empty() const { return size_ == 0; }

    inline std::size_t size() const { return size_; }

    inline void clear() {
        head_ = 0;
        size_ = 0;
    }

private:
    T buf_[N];
    std::uint8_t head_{0};
    std::uint8_t size_{0};
};
