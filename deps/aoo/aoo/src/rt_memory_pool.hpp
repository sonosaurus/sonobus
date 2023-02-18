#pragma once

#include <limits.h>
#include <stdint.h>
#include <memory>
#include <atomic>
#include <array>
#include <cassert>
#include <type_traits>

#include <iostream>

#include <stdint.h>
#include <math.h>
#include <atomic>

#include "common/utils.hpp"

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace aoo {

uint32_t clz(uint32_t i) {
#if defined(_MSC_VER)
    unsigned long msb = 0;
    _BitScanReverse(&msb, i);
    return 31 - msb;
#elif defined(__GNUC__)
    return __builtin_clz(i);
#else
#error "compiler not supported"
#endif
}

void * allocate(size_t size);
void deallocate(void *ptr, size_t);

template<typename T>
class tagged_integer {
public:
    using type = T;
    static constexpr T tag_bits = 16;
    static constexpr T value_bits = sizeof(T) * CHAR_BIT - tag_bits;
    static constexpr T tag_max = ((T)1 << tag_bits) - 1;
    static constexpr T value_max = ((T)1 << value_bits) - 1;

    tagged_integer() = default;
    tagged_integer(T tag, T value)
        : value_(((tag & tag_max) << value_bits) | (value & value_max)) {}

    void set_value(T value) {
        auto tag_part = value_ & ~value_max;
        auto value_part = value & value_max;
        value_ = tag_part | value_part;
    }

    T get_value() const {
        return value_ & value_max;
    }

    void set_tag(T tag) {
        auto value = value_ & value_max;
        value_ = ((tag & tag_max) << value_bits) | value;
    }

    T get_tag() const {
        return value_ >> value_bits;
    }

    T to_int() const {
        return value_;
    }
private:
    T value_ = 0;
};

template<typename T>
struct tagged_bitset {
    static constexpr size_t width = sizeof(T) * CHAR_BIT;
    T bits;
    uint16_t tag; // always uint16_t, so we get the same behavior across platforms

    tagged_bitset(T bits_ = 0, uint16_t tag_ = 0)
        : bits(bits_), tag(tag_) {}

    void set(T index, bool state) {
        if (state) {
            bits |= (T)1 << index;
        } else {
            bits &= ~((T)1 << index);
        }
    }

    bool get(T index) const {
        return (bits >> index) & 1;
    }

    bool empty() const {
        return bits == 0;
    }

    T highest_bit() const {
        assert(bits != 0);
        return 31 - clz(bits);
    }
};

template<typename Alloc = std::allocator<char>>
class concurrent_linear_allocator : std::allocator_traits<Alloc>::template rebind_alloc<char> {
    using alloc_type = typename std::allocator_traits<Alloc>::template rebind_alloc<char>;
public:
    concurrent_linear_allocator(const Alloc& alloc = Alloc{})
        : alloc_type(alloc) {}

    concurrent_linear_allocator(size_t size, const Alloc& alloc = Alloc{})
        : alloc_type(alloc) {
        resize(size);
    }

    concurrent_linear_allocator(concurrent_linear_allocator&& other) = delete;
    concurrent_linear_allocator& operator=(concurrent_linear_allocator&&) = delete;

    ~concurrent_linear_allocator() {
        if (alloc_size_ > 0) {
            alloc_type::deallocate(memory_, alloc_size_);
        }
    }

    void * allocate(size_t size) {
        // NB: do not use fetch_add!
        auto index = index_.load(std::memory_order_relaxed);
        while ((size_ - index) >= size) {
            if (index_.compare_exchange_weak(index, index + size,
                    std::memory_order_acquire, std::memory_order_relaxed)) {
                return data_ + index;
            } // retry and reload index
        }
        return nullptr;
    }

    void resize(size_t size) {
        if (alloc_size_ > 0) {
            alloc_type::deallocate(memory_, alloc_size_);
        }
        // align to page
        const size_t page_size = 4096;
        alloc_size_ = size + page_size;
        memory_ = alloc_type::allocate(alloc_size_);
        data_ = (char *)(((uintptr_t)memory_ + page_size - 1) & ~(page_size - 1));
        size_ = size;
        reset();
    }

    void reset() {
        index_ = 0;
    }

    size_t count() const {
        return index_.load();
    }

    bool contains(char * ptr) const {
        return ptr >= data_ && ptr < (data_ + size_);
    }

    size_t size() const {
        return size_;
    }

    const char *data() const {
        return data_;
    }
private:
    char *data_{nullptr}; // pool start (aligned to page size)
    std::atomic<size_t> index_{0}; // begin of next allocation
    size_t size_{0}; // nominal size
    char *memory_{nullptr}; // pointer to memory block
    size_t alloc_size_{0}; // size of memory block
};

using tagged_index = std::conditional_t<std::atomic<tagged_integer<uint64_t>>::is_always_lock_free,
    tagged_integer<uint64_t>, tagged_integer<uint32_t>>;

struct memory_block {
    union {
        size_t next;
        char data[1];
    };
};

// Note: we use indices into the memory arena instead of pointers
// because some platforms do not support the necessary DWCAS operations.
// For example, the ESP32 (currently) only supports 32-bit atomic operations.
class free_list {
public:
    void push(const void *pool, void *ptr) {
        auto block = static_cast<memory_block *>(ptr);
        auto index = (char *)ptr - (const char *)pool;
        auto head = head_.load(std::memory_order_relaxed);
        for (;;) {
            block->next = head.get_value();
            tagged_index new_head(head.get_tag() + 1, (size_t)index);
            if (head_.compare_exchange_weak(head, new_head,
                    std::memory_order_acq_rel, std::memory_order_relaxed)) {
                break;
            }
        }
    }

    void * pop(const void *pool) {
        auto head = head_.load(std::memory_order_relaxed);
        for (;;) {
            auto index = head.get_value();
            auto tag = head.get_tag();
            if (index == sentinel) {
                return nullptr;
            }
            auto block = reinterpret_cast<const memory_block*>((const char *)pool + index);
            tagged_index new_head(head.get_tag() + 1, block->next);
            if (head_.compare_exchange_weak(head, new_head,
                    std::memory_order_acq_rel, std::memory_order_relaxed)) {
                return (void *)block;
            }
        }
    }

    bool empty() const {
        auto head = head_.load(std::memory_order_relaxed);
        return head.get_value() != sentinel;
    }

    void reset() {
        head_ = tagged_index(0, sentinel);
    }

    size_t count(const void *pool) const {
        size_t n = 0;
        auto index = head_.load().get_value();
        while (index != sentinel) {
            auto block = reinterpret_cast<const memory_block*>((const char *)pool + index);
            index = block->next;
            n++;
        }
        return n;
    }
private:
    // NB: make sure that the sentinel fits into size_t!!
    static constexpr auto sentinel = std::min<tagged_index::type>(tagged_index::value_max, SIZE_MAX);
    std::atomic<tagged_index> head_{tagged_index{0, sentinel}};
};

#ifndef AOO_RT_MEMORY_POOL_BITSET
#define AOO_RT_MEMORY_POOL_BITSET 1
#endif

template<bool grow = true, typename Alloc = std::allocator<char>>
class rt_memory_pool : std::allocator_traits<Alloc>::template rebind_alloc<char> {
    using alloc_type = typename std::allocator_traits<Alloc>::template rebind_alloc<char>;

    static constexpr size_t block_alignment = 64;
    static constexpr size_t bucket_count = 128;
    static constexpr size_t large_bucket_offset = 64;
    static constexpr size_t small_alloc_limit = 4096;
    static constexpr size_t small_alloc_limit_bits = 12;
    static constexpr size_t large_alloc_limit = 65535;

    using bitset = std::conditional_t<std::atomic<tagged_bitset<uint32_t>>::is_always_lock_free,
        tagged_bitset<uint32_t>, tagged_bitset<uint16_t>>;
public:
    static constexpr size_t max_pool_size = std::min<tagged_index::type>(tagged_index::value_max * block_alignment, SIZE_MAX);

    rt_memory_pool(const Alloc& alloc = Alloc{})
        : alloc_type(alloc), linear_allocator_(alloc) {
        // populate bucket size array
        for (size_t i = 0; i < bucket_sizes_.size(); ++i) {
            if (i >= large_bucket_offset) {
                auto pow2 = 1 << ((i - large_bucket_offset) / 16 + small_alloc_limit_bits);
                auto rem = ((i - large_bucket_offset) % 16) * pow2 / 16;
                auto step = pow2 >> 4;
                auto size = pow2 + ((rem + step) & ~(step - 1));
                bucket_sizes_[i] = size;
            } else {
                bucket_sizes_[i] = (i + 1) * block_alignment;
            }
        }
    }

    rt_memory_pool(const rt_memory_pool&) = delete;
    rt_memory_pool& operator=(const rt_memory_pool&) = delete;

    ~rt_memory_pool() {}

    void reset() {
        linear_allocator_.reset();
        for (auto& fl : buckets_) {
            fl.reset();
        }
    }

    void resize(size_t size) {
        linear_allocator_.resize(std::min(size, max_pool_size));
        reset();
    }

    size_t size() const {
        return linear_allocator_.size();
    }

    void * allocate(size_t size) {
        if (size > large_alloc_limit) {
            LOG_VERBOSE("RT memory request (" << size << " bytes) too large - using default allocator");
            // fall back to heap allocation
            return alloc_type::allocate(size);
        } else if (size > 0) {
            auto ptr = find_block(size);
            if (!ptr) {
                auto alloc_size = get_alloc_size(size);
                ptr = linear_allocator_.allocate(alloc_size);
            }
            if (grow && !ptr) {
                if (!warned_.exchange(true, std::memory_order_relaxed)) {
                    LOG_WARNING("RT memory pool exhausted - using default allocator");
                }
                ptr = alloc_type::allocate(size);
            }
            return ptr;
        } else {
            return nullptr;
        }
    }

    void deallocate(void *ptr, size_t size) {
        if (size > large_alloc_limit) {
            // fall back to heap allocation
            alloc_type::deallocate((char *)ptr, size);
        } else if (size > 0) {
            assert(ptr != nullptr);
            if (grow && !linear_allocator_.contains((char *)ptr)) {
                alloc_type::deallocate((char *)ptr, size);
            } else {
                return_block(ptr, size);
            }
        }
    }

    void print() {
        std::cout << "total RT memory usage: " << memory_usage()
                  << " / " << size() << " bytes" << std::endl;
    #if 0
        for (size_t i = 0; i < buckets_.size(); ++i) {
            auto count = buckets_[i].count(linear_allocator_.data());
            auto bytes = count * bucket_sizes_[i];
            std::cout << "bucket " << i << ": " << count << " elements, "
                      << bytes << " bytes" << std::endl;
        }
    #endif
    }

    size_t memory_usage() const {
        return linear_allocator_.count();
    }
private:
    std::array<free_list, bucket_count> buckets_;
    std::array<uint32_t, bucket_count> bucket_sizes_;
#if AOO_RT_MEMORY_POOL_BITSET
    std::array<std::atomic<bitset>, bucket_count / bitset::width> bitset_;
#endif
    concurrent_linear_allocator<Alloc> linear_allocator_;
    std::atomic_bool warned_{false};

    size_t get_alloc_size(size_t size) {
        if (size > small_alloc_limit) {
        #if 1
            auto index = size_to_index(size);
            return bucket_sizes_[index];
        #else
            auto ilog2 = 31 - clz(size);
            auto pow2 = 1 << ilog2;
            auto mask = pow2 - 1;
            auto rem = size & mask;
            auto step = pow2 >> 4;
            return pow2 + ((rem + step) & ~(step - 1));
        #endif
        } else {
            return (size + block_alignment - 1) & ~(block_alignment - 1);
        }
    }

    static uint32_t size_to_index(uint32_t size) {
        if (size > small_alloc_limit) {
            auto ilog2 = 31 - clz(size);
            // auto pow2 = 1 << ilog2;
            // auto mask = pow2 - 1;
            // auto rem = size & mask;
            const auto k = large_bucket_offset - small_alloc_limit_bits * 16;
            // a) return (ilog2 + rem / pow2 - small_alloc_limit_bits) * 16 + large_bucket_offset;
            // b) return ilog2 * 16 + rem * 16 / pow2 + k;
            // c) return (ilog2 << 4) + ((size & ((1 << ilog2) - 1)) >> (ilog2 - 4)) + k;
            auto index = (ilog2 << 4) + ((size >> (ilog2 - 4)) & 15) + k;
            assert(index >= 64 && index < 128);
            return index;
        } else {
            // [1, 64] go to slot 0, [65 - 128] go to slot 1, etc.
            return (size - 1) / block_alignment;
        }
    }

    void * find_block(size_t size) {
        auto index = size_to_index(size);
        assert(index < buckets_.size());
        // first try to get block of matching size
        auto ptr = pop_block(index);
        if (!ptr) {
            ptr = find_matching_block(index);
        }
        return ptr;
    }

    void * find_matching_block(size_t start_index) {
        // find the largest available block above 'start_index'
    #if AOO_RT_MEMORY_POOL_BITSET
        // 1) iterate over bitsets (in reverse)
        size_t offset = start_index / bitset::width;
        for (int k = (int)bitset_.size() - 1; k >= (int)offset; --k) {
            auto bitset = bitset_[k].load(std::memory_order_relaxed);
            if (bitset.empty()) {
                continue;
            }
            // iterate over bits (in reverse)
            for (int i = bitset.highest_bit(); i >= 0; --i) {
                if (bitset.get(i)) {
                    auto block_index = k * bitset::width + i;
                    if (block_index > start_index) {
                        auto ptr = pop_block(block_index);
                        if (ptr) {
                            if (block_index >= large_bucket_offset) {
                                split_block(ptr, block_index, start_index);
                            }
                            return ptr;
                        }
                    } else {
                        return nullptr;
                    }
                }
            }
        }
    #else
        // simple linear search (in reverse)
        for (auto i = buckets_.size() - 1; i > start_index; --i) {
            auto ptr = pop_block(i);
            if (ptr) {
                if (i >= large_bucket_offset) {
                    split_block(ptr, i, start_index);
                }
                return ptr;
            }
        }
    #endif
        return nullptr;
    }

    void split_block(void *ptr, size_t block_index, size_t start_index) {
        // now split the block
        auto alloc_size = bucket_sizes_[start_index];
        auto mem_size = bucket_sizes_[block_index] - alloc_size;
        auto mem_ptr = (char *)ptr + alloc_size;
        while (mem_size > 0) {
            auto index = size_to_index(mem_size);
            if (index >= large_bucket_offset) {
                // large remainder, use lower index and split
                index--;
                push_block(mem_ptr, index);
                auto block_size = bucket_sizes_[index];
                assert(mem_size >= block_size);
                mem_ptr += block_size;
                mem_size -= block_size;
            } else {
                // small remainder - always exact match
                assert(bucket_sizes_[index] == mem_size);
                push_block(mem_ptr, index);
                break;
            }
        }
    }

    void return_block(void *ptr, size_t size) {
        auto index = size_to_index(size);
        assert(index < buckets_.size());
        assert(index < linear_allocator_.size());
        push_block(ptr, index);
    }

#if AOO_RT_MEMORY_POOL_BITSET
    void update_bitset(size_t index) {
        auto k = index / bitset::width;
        auto i = index & (bitset::width - 1);
        // set bit atomically with ABA protection.
        // this guarantees that the most recent caller writes the correct state.
        auto bs = bitset_[k].load(std::memory_order_acquire);
        for (;;) {
            bitset bs_new(bs.bits, bs.tag + 1);
            bs_new.set(i, !buckets_[index].empty());
            if (bitset_[k].compare_exchange_weak(bs, bs_new,
                    std::memory_order_acq_rel, std::memory_order_acquire)) {
                break;
            }
        }
    }
#endif

    void push_block(void *ptr, size_t index) {
        buckets_[index].push(linear_allocator_.data(), ptr);
    #if AOO_RT_MEMORY_POOL_BITSET
        update_bitset(index);
    #endif
    }

    void * pop_block(size_t index) {
        auto ptr = buckets_[index].pop(linear_allocator_.data());
    #if AOO_RT_MEMORY_POOL_BITSET
        if (ptr) {
            update_bitset(index);
        }
    #endif
        return ptr;
    }
};

} // namespace aoo
