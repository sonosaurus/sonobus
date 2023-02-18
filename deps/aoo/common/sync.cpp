/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "sync.hpp"

#ifdef _WIN32
  #include <windows.h>
#endif

namespace aoo {
namespace sync {

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

#elif defined(AOO_HAVE_PTHREAD_RWLOCK)

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

#endif // _WIN32 || AOO_HAVE_PTHREAD_RWLOCK

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

void semaphore::post() {
    auto old = count_.fetch_add(1, std::memory_order_release);
    if (old < 0){
        sem_.post();
    }
}
void semaphore::wait() {
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

void event::set() {
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

void event::wait() {
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
