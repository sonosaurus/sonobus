#include <atomic>
#include <stdio.h>

volatile std::atomic<double> d{0};

#ifdef REQUIRE_ALWAYS_LOCKFREE
#if __cplusplus >= 201703L
static_assert(std::atomic<double>::is_always_lock_free,
              "atomic double is not lockfree!");
#endif
#endif

int main() {
    d.store(1);
    if (d.is_lock_free()) {
        return 0;
    } else {
        return 1;
    }
}
