#include <pthread.h>

int main() {
    pthread_rwlock_t rwlock;
    // check if we really have all functions!
    pthread_rwlock_init(&rwlock, 0);
    pthread_rwlock_rdlock(&rwlock);
    pthread_rwlock_tryrdlock(&rwlock);
    pthread_rwlock_wrlock(&rwlock);
    pthread_rwlock_trywrlock(&rwlock);
    pthread_rwlock_unlock(&rwlock);
    pthread_rwlock_destroy(&rwlock);

    return 0;
}
