/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <inttypes.h>
#include <atomic>

#ifndef _WIN32
#include <pthread.h>
#ifdef __APPLE__
  // macOS doesn't support unnamed pthread semaphores,
  // so we use Mach semaphores instead
  #include <mach/mach.h>
#else
  // unnamed pthread semaphore
  #include <semaphore.h>
#endif
#endif

// for shared_lock
#include <shared_mutex>

namespace aoo {
namespace sync {

/*////////////////// thread priority /////////////////////*/

void lower_thread_priority();

/*////////////////// simple spin lock ////////////////////*/

class spinlock {
public:
    spinlock() = default;
    spinlock(const spinlock&) = delete;
    spinlock& operator=(const spinlock&) = delete;
    void lock();
    bool try_lock();
    void unlock();
protected:
    std::atomic<uint32_t> locked_{false};
};

/*/////////////////// shared spin lock /////////////////////////*/

class shared_spinlock {
public:
    shared_spinlock() = default;
    shared_spinlock(const shared_spinlock&) = delete;
    shared_spinlock& operator=(const shared_spinlock&) = delete;
    // exclusive
    void lock();
    bool try_lock();
    void unlock();
    // shared
    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();
protected:
    const uint32_t UNLOCKED = 0;
    const uint32_t LOCKED = 0x80000000;
    std::atomic<uint32_t> state_{0};
};

// paddeded spin locks

template<typename T, size_t N>
class alignas(N) padded_class : public T {
    // pad and align to prevent false sharing
    char pad_[N - sizeof(T)];
};

static const size_t CACHELINE_SIZE = 64;

using padded_spinlock = padded_class<spinlock, CACHELINE_SIZE>;

using padded_shared_spinlock =  padded_class<shared_spinlock, CACHELINE_SIZE>;


// The std::mutex implementation on Windows is bad on both MSVC and MinGW:
// the MSVC version apparantely has some additional overhead;
// winpthreads (MinGW) doesn't even use the obvious platform primitive (SRWLOCK),
// they rather roll their own mutex based on atomics and Events, which is bad for our use case.
//
// Older OSX versions (OSX 10.11 and below) don't have std:shared_mutex...
//
// Even on Linux, there's some overhead for things we don't need, so we use pthreads directly.

class mutex {
public:
    mutex();
    ~mutex();
    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;
    void lock();
    bool try_lock();
    void unlock();
private:
#ifdef _WIN32
    void* mutex_; // avoid including windows headers (SWRLOCK is pointer sized)
#else
    pthread_mutex_t mutex_;
#endif
};

/*//////////////////////// shared_mutex //////////////////////////*/

class shared_mutex {
public:
    shared_mutex();
    ~shared_mutex();
    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;
    // exclusive
    void lock();
    bool try_lock();
    void unlock();
    // shared
    void lock_shared();
    bool try_lock_shared();
    void unlock_shared();
private:
#ifdef _WIN32
    void* rwlock_; // avoid including windows headers (SWRLOCK is pointer sized)
#else
    pthread_rwlock_t rwlock_;
#endif
};

typedef std::try_to_lock_t try_to_lock_t;
typedef std::defer_lock_t defer_lock_t;
typedef std::adopt_lock_t adopt_lock_t;

constexpr try_to_lock_t try_to_lock {};
constexpr defer_lock_t defer_lock {};
constexpr adopt_lock_t adopt_lock {};

template<typename T>
using shared_lock = std::shared_lock<T>;

template<typename T>
using unique_lock = std::unique_lock<T>;

template<typename T>
class scoped_lock {
public:
    scoped_lock(T& lock)
        : lock_(lock){ lock_.lock(); }
    scoped_lock(const T& lock) = delete;
    scoped_lock& operator=(const T& lock) = delete;
    ~scoped_lock() { lock_.unlock(); }
private:
    T& lock_;
};

template<typename T>
class scoped_shared_lock {
public:
    scoped_shared_lock(T& lock)
        : lock_(lock){ lock_.lock_shared(); }
    scoped_shared_lock(const T& lock) = delete;
    scoped_shared_lock& operator=(const T& lock) = delete;
    ~scoped_shared_lock() { lock_.unlock_shared(); }
private:
    T& lock_;
};

/*////////////////////// semaphore ////////////////////*/

namespace detail {

class native_semaphore {
 public:
    native_semaphore();
    ~native_semaphore();
    native_semaphore(const native_semaphore&) = delete;
    native_semaphore& operator=(const native_semaphore&) = delete;
    void post();
    void wait();
 private:
#if defined(_WIN32)
    void *sem_;
#elif defined(__APPLE__)
    semaphore_t sem_;
#else // pthreads
    sem_t sem_;
#endif
};

} // detail

// thanks to https://preshing.com/20150316/semaphores-are-surprisingly-versatile/

class semaphore {
 public:
    void post(){
        auto old = count_.fetch_add(1, std::memory_order_release);
        if (old < 0){
            sem_.post();
        }
    }
    void wait(){
        auto old = count_.fetch_sub(1, std::memory_order_acquire);
        if (old <= 0){
            sem_.wait();
        }
    }
 private:
    detail::native_semaphore sem_;
    std::atomic<int32_t> count_{0};
};

/*////////////////// event ///////////////////////*/

class event {
 public:
    void set(){
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
    void wait(){
        auto old = count_.fetch_sub(1, std::memory_order_acquire);
        if (old <= 0){
            sem_.wait();
        }
    }
 private:
    detail::native_semaphore sem_;
    std::atomic<int32_t> count_{0};
};

} // sync
} // aoo
