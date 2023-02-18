#pragma once

#include <memory>
#include <atomic>

namespace aoo {

//---------------- allocator -----------------------//

#if AOO_CUSTOM_ALLOCATOR || AOO_DEBUG_MEMORY

void * allocate(size_t size);

template<class T, class... U>
T * construct(U&&... args){
    auto ptr = allocate(sizeof(T));
    return new (ptr) T(std::forward<U>(args)...);
}

void deallocate(void *ptr, size_t size);

template<typename T>
void destroy(T *x){
    if (x) {
        x->~T();
        deallocate(x, sizeof(T));
    }
}

template<class T>
class allocator {
public:
    using value_type = T;

    allocator() noexcept = default;

    template<typename U>
    allocator(const allocator<U>&) noexcept {}

    template<typename U>
    allocator& operator=(const allocator<U>&) noexcept {}

    value_type* allocate(size_t n) {
        return (value_type *)aoo::allocate(sizeof(T) * n);
    }

    void deallocate(value_type* p, size_t n) noexcept {
        aoo::deallocate(p, sizeof(T) * n);
    }
};

template <class T, class U>
bool operator==(const allocator<T>&, const allocator<U>&) noexcept
{
    return true;
}

template <class T, class U>
bool operator!=(const allocator<T>& x, const allocator<U>& y) noexcept
{
    return !(x == y);
}

#else

inline void * allocate(size_t size){
    return operator new(size);
}

template<class T, class... U>
T * construct(U&&... args){
    return new T(std::forward<U>(args)...);
}

inline void deallocate(void *ptr, size_t size){
    operator delete(ptr);
}

template<typename T>
void destroy(T *x){
    delete x;
}

template<typename T>
using allocator = std::allocator<T>;

#endif

template<typename T>
class deleter {
public:
    void operator() (void *p) const {
        aoo::deallocate(p, sizeof(T));
    }
};

//------------- RT memory allocator ---------------//

void * rt_allocate(size_t size);

template<class T, class... U>
T * rt_construct(U&&... args){
    auto ptr = rt_allocate(sizeof(T));
    return new (ptr) T(std::forward<U>(args)...);
}

void rt_deallocate(void *ptr, size_t size);

template<typename T>
void rt_destroy(T *x){
    if (x) {
        x->~T();
        rt_deallocate(x, sizeof(T));
    }
}

template<class T>
class rt_allocator {
public:
    using value_type = T;

    rt_allocator() noexcept = default;

    template<typename U>
    rt_allocator(const rt_allocator<U>&) noexcept {}

    template<typename U>
    rt_allocator& operator=(const rt_allocator<U>&) noexcept {}

    value_type* allocate(size_t n) {
        return (value_type *)aoo::rt_allocate(sizeof(T) * n);
    }

    void deallocate(value_type* p, size_t n) noexcept {
        aoo::rt_deallocate(p, sizeof(T) * n);
    }
};

template <class T, class U>
bool operator==(const rt_allocator<T>&, const rt_allocator<U>&) noexcept
{
    return true;
}

template <class T, class U>
bool operator!=(const rt_allocator<T>& x, const rt_allocator<U>& y) noexcept
{
    return !(x == y);
}

template<typename T>
class rt_deleter {
public:
    void operator() (void *p) const {
        aoo::rt_deallocate(p, sizeof(T));
    }
};

void rt_memory_pool_ref();
void rt_memory_pool_unref();

class rt_memory_pool_client {
public:
    rt_memory_pool_client() {
        rt_memory_pool_ref();
    }

    ~rt_memory_pool_client() {
        rt_memory_pool_unref();
    }
};

//------------------ memory_list --------------------//

#define DEBUG_MEMORY_LIST 0

class memory_list {
public:
    memory_list() = default;

    ~memory_list();

    memory_list(memory_list&& other)
        : list_(other.list_.exchange(nullptr)){}

    memory_list& operator=(memory_list&& other){
        list_.store(other.list_.exchange(nullptr));
        return *this;
    }

    void* allocate(size_t size);

    template<typename T, typename... Args>
    T* construct(Args&&... args){
        auto mem = allocate(sizeof(T));
        return new (mem) T (std::forward<Args>(args)...);
    }

    void deallocate(void* b);

    template<typename T>
    void destroy(T *obj){
        obj->~T();
        deallocate(obj);
    }
private:
    struct block {
        struct {
            block *next;
            size_t size;
        } header;
        char data[1];

        static block * alloc(size_t size);

        static void free(block *mem);

        static block * from_bytes(void *bytes){
            return (block *)((char *)bytes - sizeof(block::header));
        }
    };
    std::atomic<block *> list_{nullptr};
};

inline memory_list::block * memory_list::block::alloc(size_t size){
    auto fullsize = sizeof(block::header) + size;
    auto b = (block *)aoo::allocate(fullsize);
    b->header.next = nullptr;
    b->header.size = size;
#if DEBUG_MEMORY_LIST
    LOG_ALL("allocate memory block (" << size << " bytes)");
#endif
    return b;
}

inline void memory_list::block::free(memory_list::block *b){
#if DEBUG_MEMORY_LIST
    LOG_ALL("deallocate memory block (" << b->header.size << " bytes)");
#endif
    auto fullsize = sizeof(block::header) + b->header.size;
    aoo::deallocate(b, fullsize);
}

inline memory_list::~memory_list(){
    // free memory blocks
    auto b = list_.load(std::memory_order_relaxed);
    while (b){
        auto next = b->header.next;
        block::free(b);
        b = next;
    }
}

inline void* memory_list::allocate(size_t size) {
    for (;;){
        // try to pop existing block
        auto head = list_.load(std::memory_order_relaxed);
        if (head){
            auto next = head->header.next;
            if (list_.compare_exchange_weak(head, next, std::memory_order_acq_rel)){
                if (head->header.size >= size){
                #if DEBUG_MEMORY_LIST
                    LOG_ALL("reuse memory block (" << head->header.size << " bytes)");
                #endif
                    return head->data;
                } else {
                    // free block
                    block::free(head);
                }
            } else {
                // try again
                continue;
            }
        }
        // allocate new block
        return block::alloc(size)->data;
    }
}

inline void memory_list::deallocate(void* ptr) {
    auto b = block::from_bytes(ptr);
    b->header.next = list_.load(std::memory_order_relaxed);
    // check if the head has changed and update it atomically.
    // (if the CAS fails, 'next' is updated to the current head)
    while (!list_.compare_exchange_weak(b->header.next, b, std::memory_order_acq_rel)) ;
#if DEBUG_MEMORY_LIST
    LOG_ALL("return memory block (" << b->header.size << " bytes)");
#endif
}

} // namespace aoo
