#include <stdint.h>
#include <atomic>

volatile std::atomic<int64_t> i{0};

#ifdef REQUIRE_ALWAYS_LOCKFREE
#if __cplusplus >= 201703L
static_assert(std::atomic<int64_t>::is_always_lock_free,
              "atomic int64_t is not lockfree!");
#endif
#endif

int main() {
    i.store(1);

    if (i.is_lock_free()) {
        return 0;
    } else {
        return 1;
    }
}
