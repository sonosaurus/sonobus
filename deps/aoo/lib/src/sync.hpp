/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <inttypes.h>
#include <atomic>

// for shared_lock
#include <shared_mutex>
#include <mutex>

namespace aoo {

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


/*//////////////////////// shared_mutex //////////////////////////*/

// The std::mutex implementation on Windows is bad on both MSVC and MinGW:
// the MSVC version apparantely has some additional overhead; winpthreads (MinGW) doesn't even use the obvious
// platform primitive (SRWLOCK), they rather roll their own mutex based on atomics and Events, which is bad for our use case.
//
// Older OSX versions (OSX 10.11 and below) don't have std:shared_mutex...
//
// Even on Linux, there's some overhead for things we don't need, so we use pthreads directly.

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

using shared_lock = std::shared_lock<shared_mutex>;
using unique_lock = std::unique_lock<shared_mutex>;

template<typename T>
class scoped_lock {
public:
    scoped_lock(T& lock)
        : lock_(&lock){ lock_->lock(); }
    scoped_lock(const T& lock) = delete;
    scoped_lock& operator=(const T& lock) = delete;
    ~scoped_lock() { lock_->unlock(); }
private:
    T* lock_;
};

template<typename T>
class shared_scoped_lock {
public:
    shared_scoped_lock(T& lock)
        : lock_(&lock){ lock_->lock_shared(); }
    shared_scoped_lock(const T& lock) = delete;
    shared_scoped_lock& operator=(const T& lock) = delete;
    ~shared_scoped_lock() { lock_->unlock_shared(); }
private:
    T* lock_;
};

} // aoo
