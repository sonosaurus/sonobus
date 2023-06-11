#define AOO_RT_MEMORY_POOL_BITSET 1

#include "aoo/src/rt_memory_pool.hpp"
#include "common/sync.hpp"

#include <thread>
#include <vector>
#include <iostream>
#include <chrono>
#include <random>
#include <cstring>

constexpr bool use_pool = true;
constexpr bool grow_pool = true;
constexpr size_t num_threads = 16;
constexpr double duration = 1;
constexpr size_t pool_size = (size_t)1 << 26; // 64 MB
constexpr size_t max_alloc_size = (size_t)1 << 16; // 64 KB
constexpr size_t max_blocks_per_loop = 10;
constexpr size_t max_yield_count = 100;
constexpr bool use_memory_limit = false;
constexpr size_t memory_limit = sizeof(void *) == 8 ?
            (size_t)1 << 36 : SIZE_MAX; // 64 GB resp. 4 GB

std::atomic<ptrdiff_t> global_alloc_count{0};
std::atomic<ptrdiff_t> total_alloc_bytes{0};

aoo::rt_memory_pool<grow_pool> memory_pool;

void thread_function(int num) {
    std::cout << "start thread " << num << std::endl;

    std::random_device rd;
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution size_dist;
    std::uniform_int_distribution<uint32_t> yield_dist(0, max_yield_count);
    std::uniform_int_distribution<uint32_t> block_dist(0, max_blocks_per_loop);

    using seconds = std::chrono::duration<double>;

    uint8_t value = 0;
    size_t alloc_count = 0;
    std::vector<std::pair<void *, size_t>> blocks;
    blocks.reserve(max_blocks_per_loop);

    auto start = std::chrono::high_resolution_clock::now();
    for (;;) {
        // allocate blocks
        auto num_blocks = block_dist(gen);
        for (size_t i = 0; i < num_blocks; ++i) {
            auto randf = size_dist(gen);
            auto size = (size_t)(randf * randf * randf * max_alloc_size);
            auto mem = use_pool ? memory_pool.allocate(size) : ::operator new(size);
            if (mem) {
                alloc_count++;
                global_alloc_count++;
                auto total_bytes = total_alloc_bytes.fetch_add(size);
                if (use_memory_limit && total_bytes > memory_limit) {
                    goto done;
                }
                blocks.emplace_back(mem, size);
                // do something with the memory
                memset(mem, value++, size);
            }
        }
        // yield in a loop
        auto count = yield_dist(gen);
        while (count--) {
            aoo::sync::pause_cpu();
        }
        // deallocate blocks
        for (auto& block : blocks) {
            if (use_pool) {
                memory_pool.deallocate(block.first, block.second);
            } else {
                ::operator delete(block.first, block.second);
            }
        }
        blocks.clear();
        // check current time
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = seconds(now - start).count();
        if (elapsed >= duration) {
            break;
        }
    }
done:
    std::cout << "quit thread " << num << " after "
              << alloc_count << " allocations" << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "resize memory pool to " << pool_size << " bytes" << std::endl;
    memory_pool.resize(pool_size);

    std::cout << "start " << num_threads << " threads" << std::endl;
    std::cout << "---" << std::endl;

    std::vector<std::thread> threads;
    for (size_t i = 0; i < num_threads; ++i) {
        auto thread = std::thread(thread_function, i+1);
        threads.emplace_back(std::move(thread));
    }

    for (auto& thread : threads) {
        thread.join();
    }
    std::cout << "joined threads" << std::endl;
    std::cout << "---" << std::endl;
#if 1
    memory_pool.print();
    std::cout << "total number of allocations: " << global_alloc_count.load() << std::endl;
#endif
    std::cout << "done!" << std::endl;
}
