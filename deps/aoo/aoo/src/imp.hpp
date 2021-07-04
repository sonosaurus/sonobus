#pragma once

#include "aoo/aoo.h"

#include <stdint.h>
#include <utility>
#include <memory>
#include <atomic>

namespace aoo {

uint32_t make_version();

bool check_version(uint32_t version);

char * copy_string(const char *s);

void free_string(char *s);

void * copy_sockaddr(const void *sa, int32_t len);

void free_sockaddr(void *sa, int32_t len);

namespace net {

aoo_error parse_pattern(const char *msg, int32_t n, aoo_type& type, int32_t& offset);

} // net

/*///////////////////// allocator ////////////////////*/


#if AOO_CUSTOM_ALLOCATOR || AOO_DEBUG_MEMORY

void * allocate(size_t size);

template<class T, class... U>
T * construct(U&&... args){
    auto ptr = allocate(sizeof(T));
    new (ptr) T(std::forward<U>(args)...);
    return (T *)ptr;
}

void deallocate(void *ptr, size_t size);

template<typename T>
void destroy(T *x){
    x->~T();
    deallocate(x, sizeof(T));
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

    template<class U>
    struct rebind {
        typedef allocator<U> other;
    };

    value_type* allocate(size_t n) {
        return (value_type *)aoo::allocate(sizeof(T) * n);
    }

    void deallocate(value_type* p, size_t n) noexcept {
        aoo::deallocate(p, sizeof(T) * n);
    }
};

template <class T, class U>
bool operator==(allocator<T> const&, allocator<U> const&) noexcept
{
    return true;
}

template <class T, class U>
bool operator!=(allocator<T> const& x, allocator<U> const& y) noexcept
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

/*////////////// memory //////////////*/

struct memory_block {
    struct {
        memory_block *next;
        size_t size;
    } header;
    char mem[1];

    static memory_block * allocate(size_t size);

    static void free(memory_block *mem);

    static memory_block * from_bytes(void *bytes){
        return (memory_block *)((char *)bytes - sizeof(memory_block::header));
    }

    size_t full_size() const {
        return header.size + sizeof(header);
    }

    size_t size() const {
        return header.size;
    }

    void * data() {
        return mem;
    }
};

class memory_list {
public:
    memory_list() = default;
    ~memory_list();
    memory_list(memory_list&& other)
        : memlist_(other.memlist_.exchange(nullptr)){}
    memory_list& operator=(memory_list&& other){
        memlist_.store(other.memlist_.exchange(nullptr));
        return *this;
    }
    memory_block* alloc(size_t size);
    void free(memory_block* b);
private:
    std::atomic<memory_block *> memlist_{nullptr};
};

/*///////////////// misc ///////////////////*/

struct format_deleter {
    void operator() (void *x) const {
        auto f = static_cast<aoo_format *>(x);
        aoo::deallocate(x, f->size);
    }
};

} // aoo
