/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "sync.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

// for spinlock
// Intel
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
  #define CPU_INTEL
  #include <immintrin.h>
// ARM
#elif (defined(__ARM_ARCH_7__) || \
         defined(__ARM_ARCH_7A__) || \
         defined(__ARM_ARCH_7R__) || \
         defined(__ARM_ARCH_7M__) || \
         defined(__ARM_ARCH_7S__) || \
         defined(__aarch64__))
  #define CPU_ARM
#else
// fallback
  #include <thread>
#endif

namespace aoo {

void pause_cpu(){
#if defined(CPU_INTEL)
    _mm_pause();
#elif defined(CPU_ARM)
    __asm__ __volatile__("yield");
#else // fallback
    //std::this_thread::sleep_for(std::chrono::microseconds(0));
    std::this_thread::yield();
#endif
}

/*/////////////////////// spinlock //////////////////////////*/

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

/*//////////////////// shared spinlock ///////////////////////*/

// exclusive
void shared_spinlock::lock(){
    // only try to modify the shared state if the lock seems to be available.
    // this should prevent unnecessary cache invalidation.
    do {
        while (state_.load(std::memory_order_relaxed) != UNLOCKED){
            pause_cpu();
        }
    } while (!try_lock());
}

bool shared_spinlock::try_lock(){
    // check if state is UNLOCKED and set to LOCKED on success.
    uint32_t expected = UNLOCKED;
    return state_.compare_exchange_strong(expected, LOCKED, std::memory_order_acq_rel);
}

void shared_spinlock::unlock(){
    // set to UNLOCKED
    state_.store(UNLOCKED, std::memory_order_release);
}

// shared
void shared_spinlock::lock_shared(){
    while (!try_lock_shared()){
        pause_cpu();
    }
}

bool shared_spinlock::try_lock_shared(){
    // optimistically increment the reader count and then
    // check whether the LOCKED bit is *not* set,
    // otherwise we simply decrement the reader count again.
    // this is optimized for the likely case that there's no writer.
    auto state = state_.fetch_add(1, std::memory_order_acquire);
    if (!(state & LOCKED)){
        return true;
    } else {
        state_.fetch_sub(1, std::memory_order_release);
        return false;
    }
}

void shared_spinlock::unlock_shared(){
    // decrement the reader count
    state_.fetch_sub(1, std::memory_order_release);
}

/*////////////////////// shared_mutex //////////////////////*/

#ifdef _WIN32
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
#else
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
#endif

} // aoo
