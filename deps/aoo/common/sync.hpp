/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <inttypes.h>
#include <atomic>

#ifndef _WIN32
  #include <pthread.h>
#endif

#ifdef __APPLE__
// macOS doesn't support unnamed posix semaphores,
// so we use Mach semaphores instead
#include <mach/mach.h>
#endif

#if defined(__linux__) || defined(__FreeBSD__) || \
    defined(__NetBSD__) || defined(__OpenBSD__)
  #define HAVE_POSIX_SEMAPHORE
  #include <semaphore.h>
#endif

#if defined(_WIN32) || defined(__APPLE__) || defined(HAVE_POSIX_SEMAPHORE)
  #define HAVE_SEMAPHORE
#endif

// for shared_lock
#include <mutex>
#include <shared_mutex>

namespace aoo {
namespace sync {

//-------------- thread priority ------------------//

void lower_thread_priority();

//----------------- relaxed atomics ---------------//

namespace detail {
#if !defined(AOO_HAVE_ATOMIC_DOUBLE) || !defined(AOO_HAVE_ATOMIC_INT64)
// emulate atomics using a global spinlock
void global_spinlock_lock();
void global_spinlock_unlock();

template<typename T>
class relaxed_atomic_emulated {
private:
    struct scoped_lock {
        scoped_lock() { global_spinlock_lock(); }
        ~scoped_lock() { global_spinlock_unlock(); }
    };
public:
    relaxed_atomic_emulated(T value = T{}) : value_(value) {}

    T load() const {
        scoped_lock lock;
        return value_;
    }

    void store(T value) {
        scoped_lock lock;
        value_ = value;
    }

    T exchange(T value) {
        scoped_lock lock;
        return std::exchange(value_, value);
    }
private:
    T value_;
};
#endif

// native atomics
template<typename T>
class relaxed_atomic_native {
public:
    relaxed_atomic_native(T value = T{}) : value_(value) {}

    T load() const {
        return value_.load(std::memory_order_relaxed);
    }

    void store(T value) {
        value_.store(value, std::memory_order_relaxed);
    }

    T exchange(T value) {
        return value_.exchange(value, std::memory_order_relaxed);
    }
private:
#if __cplusplus >= 201703L
    static_assert(std::atomic<T>::is_always_lock_free,
                  "std::atomic<T> is not lockfree!");
#endif
    std::atomic<T> value_;
};

} // detail

template<typename T, typename enable = void>
class relaxed_atomic
        : public detail::relaxed_atomic_native<T>
{
    using detail::relaxed_atomic_native<T>::relaxed_atomic_native;
};

// specialization for doubles (if needed)
#ifndef AOO_HAVE_ATOMIC_DOUBLE
// #warning "emulating atomic doubles in software"
template<>
class relaxed_atomic<double>
        : public detail::relaxed_atomic_emulated<double>
{
    using relaxed_atomic_emulated::relaxed_atomic_emulated;
};
#endif

// specialization for 64-bit integers (if needed)
#ifndef AOO_HAVE_ATOMIC_INT64
// #warning "emulating atomic 64-bit integers in software"
template<typename T>
class relaxed_atomic<T, typename std::enable_if<
        std::is_integral<T>::value && (sizeof(T) == 8)>::type>
        : public detail::relaxed_atomic_emulated<T>
{
    using detail::relaxed_atomic_emulated<T>::relaxed_atomic_emulated;
};
#endif


//----------------- spinlock ----------------------//

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

//------------- shared spin lock -----------------//

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

//--------------- padded spin locks --------------------//

template<typename T, size_t N>
class alignas(N) padded_class : public T {
    // pad and align to prevent false sharing
    char pad_[N - sizeof(T)];
};

static const size_t CACHELINE_SIZE = 64;

using padded_spinlock = padded_class<spinlock, CACHELINE_SIZE>;

using padded_shared_spinlock =  padded_class<shared_spinlock, CACHELINE_SIZE>;

//------------------------------ mutex ------------------------------------//

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

//------------------------ shared_mutex -------------------------//

#if defined(_WIN32) || defined(AOO_HAVE_PTHREAD_RWLOCK)

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

#else
// fallback
using shared_mutex = std::shared_mutex;

#endif // _WIN32 || AOO_HAVE_PTHREAD_RWLOCK

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
    scoped_lock(const scoped_lock& lock) = delete;
    scoped_lock& operator=(const scoped_lock& lock) = delete;
    ~scoped_lock() { lock_.unlock(); }
private:
    T& lock_;
};

template<typename T>
class scoped_shared_lock {
public:
    scoped_shared_lock(T& lock)
        : lock_(lock){ lock_.lock_shared(); }
    scoped_shared_lock(const scoped_shared_lock& lock) = delete;
    scoped_shared_lock& operator=(const scoped_shared_lock& lock) = delete;
    ~scoped_shared_lock() { lock_.unlock_shared(); }
private:
    T& lock_;
};

//----------------------- semaphore --------------------------//

#ifdef HAVE_SEMAPHORE

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
#else // posix
    sem_t sem_;
#endif
};

} // detail

#endif // HAVE_SEMAPHORE

// thanks to https://preshing.com/20150316/semaphores-are-surprisingly-versatile/

class semaphore {
 public:
    semaphore();
    ~semaphore();
    semaphore(const semaphore&) = delete;
    semaphore& operator=(const semaphore&) = delete;
    void post();
    void wait();
 private:
#ifdef HAVE_SEMAPHORE
    detail::native_semaphore sem_;
    std::atomic<int32_t> count_{0};
#else
    // fallback using mutex + condition variable
    pthread_mutex_t mutex_;
    pthread_cond_t condition_;
    int32_t count_{0};
#endif
};

//------------------------- event ------------------------------//

class event {
 public:
    event();
    ~event();
    event(const event&) = delete;
    event& operator=(const event&) = delete;
    void set();
    void wait();
 private:
#ifdef HAVE_SEMAPHORE
    detail::native_semaphore sem_;
    std::atomic<int32_t> count_{0};
#else
    // fallback using mutex + condition variable
    pthread_mutex_t mutex_;
    pthread_cond_t condition_;
    bool state_{false};
#endif // HAVE_SEMAPHORE
};

} // sync
} // aoo
