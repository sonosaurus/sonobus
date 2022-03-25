/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "sync.hpp"

#ifdef _WIN32
  #include <windows.h>
#endif

// for spinlock
// Intel
#if defined(__i386__) || defined(_M_IX86) || \
    defined(__x86_64__) || defined(_M_X64)
  #define HAVE_PAUSE
  #include <immintrin.h>
// ARM
#elif (defined(__ARM_ARCH_6K__) || \
       defined(__ARM_ARCH_6Z__) || \
       defined(__ARM_ARCH_6ZK__) || \
       defined(__ARM_ARCH_6T2__) || \
       defined(__ARM_ARCH_7__) || \
       defined(__ARM_ARCH_7A__) || \
       defined(__ARM_ARCH_7R__) || \
       defined(__ARM_ARCH_7M__) || \
       defined(__ARM_ARCH_7S__) || \
       defined(__ARM_ARCH_8A__) || \
       defined(__aarch64__))
// mnemonic 'yield' is supported from ARMv6k onwards
  #define HAVE_YIELD
#else
// fallback
  #include <thread>
#endif

namespace aoo {
namespace sync {

void pause_cpu(){
#if defined(HAVE_PAUSE)
    _mm_pause();
#elif defined(HAVE_YIELD)
    __asm__ __volatile__("yield");
#else // fallback
  #warning "architecture does not support yield/pause instruction"
  #if 0
    std::this_thread::sleep_for(std::chrono::microseconds(0));
  #else
    std::this_thread::yield();
  #endif
#endif
}

//-------------------------- thread priority -----------------------------//

void lower_thread_priority()
{
#ifdef _WIN32
    // lower thread priority only for high priority or real time processes
    DWORD cls = GetPriorityClass(GetCurrentProcess());
    if (cls == HIGH_PRIORITY_CLASS || cls == REALTIME_PRIORITY_CLASS){
        int priority = GetThreadPriority(GetCurrentThread());
        SetThreadPriority(GetCurrentThread(), priority - 2);
    }
#else

#endif
}

//---------------------------- atomics -------------------------------------//

namespace detail {
static padded_spinlock g_atomic_spinlock;

void global_spinlock_lock() { g_atomic_spinlock.lock(); }

void global_spinlock_unlock() { g_atomic_spinlock.unlock(); }

}

//---------------------------- spinlock ------------------------------------//

void spinlock::lock(){
    // only try to modify the shared state if the lock seems to be available.
    // this should prevent unnecessary cache invalidation.
    do {
        while (locked_.load(std::memory_order_relaxed)){
            pause_cpu();
        }
    } while (!try_lock());
}

bool spinlock::try_lock(){
    // if the previous value was false, it means be could aquire the lock.
    // this is faster than a CAS loop.
    return !locked_.exchange(true, std::memory_order_acquire);
}

void spinlock::unlock(){
    locked_.store(false, std::memory_order_release);
}

//-------------------------- shared spinlock ---------------------------//

// exclusive
void shared_spinlock::lock(){
    // only try to modify the shared state if the lock seems to be available.
    // this should prevent unnecessary cache invalidation.
    for (;;) {
        if (state_.load(std::memory_order_relaxed) == UNLOCKED){
            // check if state is UNLOCKED and set LOCKED bit on success.
            uint32_t expected = UNLOCKED;
            if (state_.compare_exchange_weak(expected, LOCKED,
                std::memory_order_acquire, std::memory_order_relaxed)) return;
            // CAS failed -> retry immediately
        } else {
            pause_cpu();
        }
    }
}

bool shared_spinlock::try_lock(){
    // check if state is UNLOCKED and set LOCKED bit on success.
    uint32_t expected = UNLOCKED;
    return state_.compare_exchange_strong(expected, LOCKED,
        std::memory_order_acquire, std::memory_order_relaxed);
}

void shared_spinlock::unlock(){
    // clear LOCKED bit, see try_lock_shared()
    state_.fetch_and(~LOCKED, std::memory_order_release);
}

// shared
bool shared_spinlock::try_lock_shared(){
    // optimistically increment the reader count and then
    // check whether the LOCKED bit is *not* set, otherwise
    // we simply decrement the reader count again. this is
    // optimized for the likely case that there's no writer.
    auto state = state_.fetch_add(1, std::memory_order_acquire);
    if (!(state & LOCKED)){
        return true;
    } else {
        state_.fetch_sub(1, std::memory_order_acq_rel); // memory order?
        return false;
    }
}

void shared_spinlock::lock_shared(){
    // only try to modify the shared state if the lock seems to be
    // available. this should prevent unnecessary cache invalidation.
    for (;;)
    {
        auto state = state_.load(std::memory_order_relaxed);
        if (!(state & LOCKED) && try_lock_shared()) {
            return;
        } else {
            pause_cpu();
        }
    }
}

void shared_spinlock::unlock_shared(){
    // decrement the reader count
    state_.fetch_sub(1, std::memory_order_release);
}

//---------------------- mutex -------------------------//

#ifdef _WIN32

mutex::mutex() {
    InitializeSRWLock((PSRWLOCK)& mutex_);
}
mutex::~mutex() {}

void mutex::lock() {
    AcquireSRWLockExclusive((PSRWLOCK)&mutex_);
}
bool mutex::try_lock() {
    return TryAcquireSRWLockExclusive((PSRWLOCK)&mutex_);
}
void mutex::unlock() {
    ReleaseSRWLockExclusive((PSRWLOCK)&mutex_);
}

#else

mutex::mutex() {
    pthread_mutex_init(&mutex_, nullptr);
}
mutex::~mutex() {
    pthread_mutex_destroy(&mutex_);
}
void mutex::lock() {
    pthread_mutex_lock(&mutex_);
}
bool mutex::try_lock() {
    return pthread_mutex_trylock(&mutex_) == 0;
}
void mutex::unlock() {
    pthread_mutex_unlock(&mutex_);
}

#endif

//-------------------- shared_mutex -------------------------//

#if defined(_WIN32)

shared_mutex::shared_mutex() {
    InitializeSRWLock((PSRWLOCK)& rwlock_);
}
shared_mutex::~shared_mutex() {}
// exclusive
void shared_mutex::lock() {
    AcquireSRWLockExclusive((PSRWLOCK)&rwlock_);
}
bool shared_mutex::try_lock() {
    return TryAcquireSRWLockExclusive((PSRWLOCK)&rwlock_);
}
void shared_mutex::unlock() {
    ReleaseSRWLockExclusive((PSRWLOCK)&rwlock_);
}
// shared
void shared_mutex::lock_shared() {
    AcquireSRWLockShared((PSRWLOCK)&rwlock_);
}
bool shared_mutex::try_lock_shared() {
    return TryAcquireSRWLockShared((PSRWLOCK)&rwlock_);
}
void shared_mutex::unlock_shared() {
    ReleaseSRWLockShared((PSRWLOCK)&rwlock_);
}

#elif defined(HAVE_PTHREAD_RWLOCK)

shared_mutex::shared_mutex() {
    pthread_rwlock_init(&rwlock_, nullptr);
}
shared_mutex::~shared_mutex() {
    pthread_rwlock_destroy(&rwlock_);
}
// exclusive
void shared_mutex::lock() {
    pthread_rwlock_wrlock(&rwlock_);
}
bool shared_mutex::try_lock() {
    return pthread_rwlock_trywrlock(&rwlock_) == 0;
}
void shared_mutex::unlock() {
    pthread_rwlock_unlock(&rwlock_);
}
// shared
void shared_mutex::lock_shared() {
    pthread_rwlock_rdlock(&rwlock_);
}
bool shared_mutex::try_lock_shared() {
    return pthread_rwlock_tryrdlock(&rwlock_) == 0;
}
void shared_mutex::unlock_shared() {
    pthread_rwlock_unlock(&rwlock_);
}

#endif // _WIN32 || HAVE_PTHREAD_RWLOCK

//-------------------- native_semaphore -----------------------//

#ifdef HAVE_SEMAPHORE

namespace detail {

native_semaphore::native_semaphore(){
#if defined(_WIN32)
    sem_ = CreateSemaphoreA(0, 0, LONG_MAX, 0);
#elif defined(__APPLE__)
    semaphore_create(mach_task_self(), &sem_, SYNC_POLICY_FIFO, 0);
#else // posix
    sem_init(&sem_, 0, 0);
#endif
}

native_semaphore::~native_semaphore(){
#if defined(_WIN32)
    CloseHandle(sem_);
#elif defined(__APPLE__)
    semaphore_destroy(mach_task_self(), sem_);
#else // posix
    sem_destroy(&sem_);
#endif
}

void native_semaphore::post(){
#if defined(_WIN32)
    ReleaseSemaphore(sem_, 1, 0);
#elif defined(__APPLE__)
    semaphore_signal(sem_);
#else // posix
    sem_post(&sem_);
#endif
}

void native_semaphore::wait(){
#if defined(_WIN32)
    WaitForSingleObject(sem_, INFINITE);
#elif defined(__APPLE__)
    semaphore_wait(sem_);
#else // posix
    while (sem_wait(&sem_) == -1 && errno == EINTR) continue;
#endif
}

} // detail

#endif // HAVE_SEMAPHORE

//---------------------- semaphore ---------------------//

#ifdef HAVE_SEMAPHORE

semaphore::semaphore() {}

semaphore::~semaphore() {}

void semaphore::post(){
    auto old = count_.fetch_add(1, std::memory_order_release);
    if (old < 0){
        sem_.post();
    }
}
void semaphore::wait(){
    auto old = count_.fetch_sub(1, std::memory_order_acquire);
    if (old <= 0){
        sem_.wait();
    }
}

#else

semaphore::semaphore() {
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&condition_, nullptr);
}

semaphore::~semaphore() {
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&condition_);
}

void semaphore::post() {
    pthread_mutex_lock(&mutex_);
    count_++;
    pthread_mutex_unlock(&mutex_);
    pthread_cond_signal(&condition_);
}
void semaphore::wait() {
    pthread_mutex_lock(&mutex_);
    // wait till count is larger than zero
    while (count_ <= 0) {
        pthread_cond_wait(&condition_, &mutex_);
    }
    count_--; // release
    pthread_mutex_unlock(&mutex_);
}

#endif // HAVE_SEMAPHORE

//---------------------- event -------------------------//

#ifdef HAVE_SEMAPHORE

event::event() {}

event::~event() {}

void event::set(){
    int oldcount = count_.load(std::memory_order_relaxed);
    for (;;) {
        // don't increment past 1
        // NOTE: we have to use the CAS loop even if we don't
        // increment 'oldcount', because a another thread
        // might decrement the counter concurrently!
        auto newcount = oldcount >= 0 ? 1 : oldcount + 1;
        if (count_.compare_exchange_weak(oldcount, newcount, std::memory_order_release,
                                         std::memory_order_relaxed))
            break;
    }
    if (oldcount < 0)
        sem_.post(); // release one waiting thread
}

void event::wait(){
    auto old = count_.fetch_sub(1, std::memory_order_acquire);
    if (old <= 0){
        sem_.wait();
    }
}

#else

event::event() {
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&condition_, nullptr);
}

event::~event() {
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&condition_);
}

void event::set() {
    pthread_mutex_lock(&mutex_);
    state_ = true;
    pthread_mutex_unlock(&mutex_);
    pthread_cond_signal(&condition_);
}
void event::wait() {
    pthread_mutex_lock(&mutex_);
    // wait till event is set
    while (state_ != true) {
        pthread_cond_wait(&condition_, &mutex_);
    }
    state_ = false; // unset
    pthread_mutex_unlock(&mutex_);
}

#endif // HAVE_SEMAPHORE

} // sync
} // aoo
