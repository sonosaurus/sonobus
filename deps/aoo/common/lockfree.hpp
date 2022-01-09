/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <stdint.h>
#include <atomic>
#include <vector>
#include <cassert>

namespace aoo {
namespace lockfree {

//-------------------- spsc_queue -----------------------//

// a lock-free single-producer/single-consumer queue which
// supports reading/writing data in fixed-sized blocks.
template<typename T, typename Alloc=std::allocator<T>>
class spsc_queue {
 public:
    spsc_queue(const Alloc& alloc = Alloc {})
        : data_(alloc) {}

    spsc_queue(const spsc_queue& other) = delete;

    // we need a move constructor so we can
    // put it in STL containers
    spsc_queue(spsc_queue&& other)
        : Alloc(static_cast<Alloc&&>(other)),
          balance_(other.balance_.load()),
          rdhead_(other.rdhead_),
          wrhead_(other.wrhead_),
          blocksize_(other.blocksize_),
          data_(std::move(other.data_)) {}

    spsc_queue& operator=(spsc_queue&& other){
        static_cast<Alloc&>(*this) = static_cast<Alloc&>(other);
        balance_ = other.balance_.load();
        rdhead_ = other.rdhead_;
        wrhead_ = other.wrhead_;
        blocksize_ = other.blocksize_;
        data_ = std::move(other.data_);
        return *this;
    }

    void resize(int32_t blocksize, int32_t capacity) {
    #if 0
        data_.clear(); // force zero
    #endif
        data_.resize(blocksize * capacity);
        capacity_ = capacity;
        blocksize_ = blocksize;
        reset();
    }

    void resize(int32_t capacity){
        resize(1, capacity);
    }

    void shrink_to_fit(){
        data_.shrink_to_fit();
    }

    int32_t blocksize() const { return blocksize_; }

    // max. number of *blocks*
    int32_t capacity() const {
        return capacity_;
    }

    void reset() {
        rdhead_ = wrhead_ = 0;
        balance_ = 0;
    }
    // returns: the number of available *blocks* for reading
    int32_t read_available() const {
        return balance_.load(std::memory_order_relaxed);
    }

    void read(T& out) {
        out = std::move(data_[rdhead_]);
        read_commit(1);
    }

    const T* read_data() const {
        return &data_[rdhead_];
    }

    void read_commit() {
        read_commit(blocksize_);
    }

    template<typename Fn>
    void consume(Fn&& func) {
        func(data_[rdhead_]);
        read_commit(1);
    }

    template<typename Fn>
    void consume_all(Fn&& func) {
        while (read_available() > 0){
            consume(std::forward<Fn>(func));
        }
    }

    // returns: the number of available *blocks* for writing
    int32_t write_available() const {
        return capacity_ - balance_.load(std::memory_order_relaxed);
    }

    template<typename U>
    void write(U&& value) {
        data_[wrhead_] = std::forward<U>(value);
        write_commit(1);
    }

    T* write_data() {
        return &data_[wrhead_];
    }

    void write_commit() {
        write_commit(blocksize_);
    }

    template<typename Fn>
    void produce(Fn&& func) {
        func(data_[wrhead_]);
        write_commit(1);
    }
 private:
    std::vector<T, Alloc> data_;
    std::atomic<int32_t> balance_{0};
    int32_t rdhead_{0};
    int32_t wrhead_{0};
    int32_t blocksize_{0};
    int32_t capacity_{0};

    void read_commit(int32_t n){
        rdhead_ += n;
        if (rdhead_ == data_.size()){
            rdhead_ = 0;
        }
        auto b = balance_.fetch_sub(1, std::memory_order_release);
        assert(b > 0);
    }

    void write_commit(int32_t n){
        wrhead_ += n;
        if (wrhead_ == data_.size()){
            wrhead_ = 0;
        }
        auto b = balance_.fetch_add(1, std::memory_order_release);
        assert(b < capacity_);
    }
};

//--------------------- unbounded_mpsc_queue -------------------------//

// based on https://www.drdobbs.com/parallel/writing-lock-free-code-a-corrected-queue/210604448

namespace detail {

template<typename T>
class node_base {
public:
    struct node {
        template<typename... U>
        node(U&&... args)
            : next_(nullptr), data_(std::forward<U>(args)...) {}
        node * next_;
        T data_;
    };
};

template<typename T>
struct atomic_node_base {
    struct node {
        std::atomic<node*> next_;
        T data_;

        template<typename... U>
        node(U&&... args)
            : next_(nullptr), data_(std::forward<U>(args)...) {}
    };
};

template<typename C, typename Alloc>
class node_allocator_base :
        protected C,
        protected Alloc::template rebind<typename C::node>::other
{
    typedef typename Alloc::template rebind<typename C::node>::other base;
protected:
    typedef typename C::node node;

    node_allocator_base(const Alloc& alloc)
        : base(alloc) {}

    node * allocate(){
        return base::allocate(1);
    }

    void deallocate(node *n){
        base::deallocate(n, 1);
    }
};

} // detail

template<typename T, typename Alloc = std::allocator<T>>
class unbounded_mpsc_queue :
    detail::node_allocator_base<detail::node_base<T>, Alloc>
{
    typedef detail::node_allocator_base<detail::node_base<T>, Alloc> base;
    typedef typename base::node node;
 public:
    unbounded_mpsc_queue(const Alloc& alloc = Alloc{})
        : base(alloc) {
        // add dummy node
        auto n = base::allocate();
        new (n) node();
        first_ = devider_ = last_ = n;
    }

    unbounded_mpsc_queue(const unbounded_mpsc_queue&) = delete;

    unbounded_mpsc_queue(unbounded_mpsc_queue&& other)
        : base(std::move(other))
    {
        first_ = other.first_;
        devider_ = other.devider_;
        last_ = other.last_;
        other.first_ = nullptr;
        other.devider_ = nullptr;
        other.last_ = nullptr;
    }

    unbounded_mpsc_queue& operator=(unbounded_mpsc_queue&& other){
        base::operator=(std::move(other));
        first_ = other.first_;
        devider_ = other.devider_;
        last_ = other.last_;
        other.first_ = nullptr;
        other.devider_ = nullptr;
        other.last_ = nullptr;
        return *this;
    }

    ~unbounded_mpsc_queue(){
        auto it = first_.load();
        while (it){
            auto tmp = it;
            it = it->next_;
            tmp->~node();
            base::deallocate(tmp);
        }
    }

    // not thread-safe!
    void reserve(size_t n){
        // check for existing empty nodes
        auto it = first_.load();
        auto end = devider_.load();
        while (it != end){
            n--;
            it = it->next_;
        }
        // add empty nodes
        while (n--){
            auto tmp = base::allocate();
            new (tmp) node();
            tmp->next_ = first_;
            first_.store(tmp);
        }
    }

    // can be called by several threads
    template<typename... U>
    void push(U&&... args){
        auto n = get_node();
        n->data_ = T{std::forward<U>(args)...};
        push_node(n);
    }

    template<typename Fn>
    void produce(Fn&& func){
        auto n = get_node();
        func(n->data_);
        push_node(n);
    }

    // must be called from a single thread!
    void pop(T& result){
        // use node *after* devider, because devider is always a dummy!
        auto next = devider_.load(std::memory_order_relaxed)->next_;
        result = std::move(next->data_);
        devider_.store(next, std::memory_order_release); // publish
    }

    bool try_pop(T& result){
        if (!empty()){
            pop(result);
            return true;
        } else {
            return false;
        }
    }

    template<typename Fn>
    void consume(Fn&& func){
        // use node *after* devider, because devider is always a dummy!
        auto next = devider_.load(std::memory_order_relaxed)->next_;
        func(next->data_);
        devider_.store(next, std::memory_order_release); // publish
    }

    template<typename Fn>
    bool try_consume(Fn&& func){
        if (!empty()){
            consume(std::forward<Fn>(func));
            return true;
        } else {
            return false;
        }
    }

    template<typename Fn>
    void consume_all(Fn&& func){
        while (!empty()){
            consume(std::forward<Fn>(func));
        }
    }

    bool empty() const {
        return devider_.load(std::memory_order_relaxed)
                == last_.load(std::memory_order_relaxed);
    }

    // not thread-safe (?)
    void clear(){
        devider_.store(last_);
    }
 private:
    std::atomic<node *> first_;
    std::atomic<node *> devider_;
    std::atomic<node *> last_;
    std::atomic<int32_t> lock_{0};

    node * get_node(){
        for (;;){
            auto first = first_.load(std::memory_order_relaxed);
            if (first != devider_.load(std::memory_order_relaxed)){
                // try to reuse existing node
                if (first_.compare_exchange_weak(first, first->next_,
                                                 std::memory_order_acq_rel))
                {
                    first->next_ = nullptr; // !
                    return first;
                }
            } else {
                // make new node
                auto n = base::allocate();
                new (n) node();
                return n;
            }
        }
    }

    void push_node(node *n){
        while (lock_.exchange(1, std::memory_order_acquire)) ; // lock
        auto last = last_.load(std::memory_order_relaxed);
        last->next_ = n;
        last_.store(n, std::memory_order_release); // publish
        lock_.store(0, std::memory_order_release); // unlock
    }
};

//------------------------ concurrent_list ---------------------------------//

// A lock-free singly-linked list.
// Adding/removing items and iterating over the list is generally thread-safe,
// but with the following restrictions:
// * nodes must only be removed from a single thread
// * each thread trying to access the list must call lock()/unlock(),
//   so that try_remove() knows when it is safe to actually free the memory.

template<typename T, typename Alloc = std::allocator<T>>
class concurrent_list :
    detail::node_allocator_base<detail::atomic_node_base<T>, Alloc>
{
    typedef detail::node_allocator_base<detail::atomic_node_base<T>, Alloc> base;
    typedef typename base::node node;
public:
    template<typename U>
    class base_iterator {
        friend class concurrent_list;
        U *node_;
    public:
        typedef std::ptrdiff_t difference_type;
        typedef U value_type;
        typedef U& reference;
        typedef U* pointer;
        typedef std::forward_iterator_tag iterator_category;

        base_iterator()
            : node_(nullptr){}
        base_iterator(U *n)
            : node_(n){}
        base_iterator(const base_iterator&) = default;
        base_iterator& operator=(const base_iterator&) = default;
        T& operator*() { return node_->data_; }
        T* operator->() { return &node_->data_; }
        base_iterator& operator++() {
            node_ = node_->next_.load(std::memory_order_acquire);
            return *this;
        }
        base_iterator operator++(int) {
            base_iterator old(node_);
            node_ = node_->next_.load(std::memory_order_acquire);
            return old;
        }
        bool operator==(const base_iterator& other){
            return node_ == other.node_;
        }
        bool operator!=(const base_iterator& other){
            return node_ != other.node_;
        }
    };
    using iterator = base_iterator<node>;
    using const_iterator = base_iterator<const node>;

    concurrent_list(const Alloc& alloc = Alloc{})
        : base(alloc) {}

    concurrent_list(const concurrent_list&) = delete;

    concurrent_list(concurrent_list&& other)
        : base(std::move(other))
    {
        head_ = other.head_.exchange(nullptr);
        free_ = other.free_.exchange(nullptr);
        refcount_ = other.refcount_.exchange(0);
    }

    ~concurrent_list(){
        destroy_list(head_.load());
        destroy_list(free_.load());
    }

    concurrent_list& operator=(concurrent_list&& other){
        base::operator=(std::move(other));
        head_ = other.head_.exchange(nullptr);
        free_ = other.free_.exchange(nullptr);
        refcount_ = other.refcount_.exchange(0);
        return *this;
    }

    template<typename... U>
    iterator emplace_front(U&&... args){
        auto n = base::allocate();
        new (n) node(std::forward<U>(args)...);
        auto next = head_.load(std::memory_order_relaxed);
        do {
            n->next_.store(next, std::memory_order_relaxed);
            // check if the head has changed and update it atomically.
            // (if the CAS fails, 'next' is updated to the current head)
        } while (!head_.compare_exchange_weak(next, n, std::memory_order_acq_rel)) ;
        return iterator(n);
    }

    iterator push_front(T&& v){
        return emplace_front(std::forward<T>(v));
    }

    // NOTE: don't call concurrently!
    void pop_front(){
        T *head = head_.load(std::memory_order_relaxed);
        T *next;
        do {
            next = head->next_.load(std::memory_order_relaxed);
            // check if the head has changed and update it atomically.
            // (if the CAS fails, 'head' is updated to the current head)
        } while (!head_.compare_exchange_weak(head, next, std::memory_order_acq_rel));

        dispose_node(head);
    }

    // NOTE: don't call concurrently!
    iterator erase(iterator it){
        for (;;){
            auto n = head_.load(std::memory_order_acquire);
            if (n == it.node_){
                // try to remove head
                auto next = n->next_.load(std::memory_order_acquire);
                if (head_.compare_exchange_strong(n, next, std::memory_order_acq_rel)){
                    dispose_node(n);
                    return iterator(next); // success
                }
                // someone pushed a new node in between, try again!
            } else {
                // find the node before it
                while (n){
                    auto next = n->next_.load(std::memory_order_acquire);
                    if (next == it.node_){
                        // atomically unlink node
                        auto next2 = next->next_.load(std::memory_order_acquire);
                    #if 0
                        if (n->next_.compare_exchange_strong(next, next2, std::memory_order_acq_rel)){
                            dispose_node(next);
                            return iterator(next2);
                        } else {
                            // someone concurrently removed 'it'(shouldn't happen)
                            return iterator{};
                        }
                    #else
                        n->next_.store(next2, std::memory_order_release);
                        dispose_node(next);
                        return iterator(next2);
                    #endif
                    }
                    n = next;
                }
                // reached end of list, 'it' might have been removed concurrently (shouldn't happen)
                return iterator{};
            }
        }
    }

    T& front() { return *begin(); }

    T& front() const { return *begin(); }

    iterator begin(){
        return iterator(head_.load(std::memory_order_acquire));
    }

    const_iterator begin() const {
        return const_iterator(head_.load(std::memory_order_acquire));
    }

    iterator end(){
        return iterator();
    }

    const_iterator end() const {
        return const_iterator();
    }

    bool empty() const {
        return head_.load(std::memory_order_relaxed) == nullptr;
    }

    void clear(){
        // atomically unlink the whole list
        auto head = head_.exchange(nullptr);
        if (head){
            // and move it to the free list
            append_list(head, free_);
        }
    }

    void lock(){
        refcount_.fetch_add(1, std::memory_order_acquire);
    }

    void unlock(){
        refcount_.fetch_sub(1, std::memory_order_release);
    }

    // always call in unlocked state!
    bool try_free(){
        // only try to free if the refcount is zero
        if (!refcount_.load(std::memory_order_relaxed)){
            // atomically unlink the whole free list
            auto f = free_.exchange(nullptr); // relaxed?
            if (!f){
                return true; // nothing to free
            }
            // now really check the refcount.
            // after this point it can safely go up again,
            // because if won't affect the old free list.
            if (!refcount_.load(std::memory_order_acquire)){
                // free memory
                destroy_list(f);
                return true;
            } else {
                // put it back to the free list
                append_list(f, free_);
            }
        }
        return false;
    }
private:
    std::atomic<node *> head_{nullptr};
    std::atomic<node *> free_{nullptr};
    std::atomic<int32_t> refcount_{0};

    void dispose_node(node * n){
        // atomically add node to free list
        auto next = free_.load(std::memory_order_relaxed);
        do {
            n->next_.store(next, std::memory_order_relaxed);
            // check if the head has changed and update it atomically.
            // (if the CAS fails, 'next' is updated to the current head)
        } while (!free_.compare_exchange_weak(next, n, std::memory_order_acq_rel));
    }

    void append_list(node *src, std::atomic<node *>& dst){
        // find last node
        auto n = src;
        for (;;){
            auto next = n->next_.load(std::memory_order_relaxed);
            if (!next){
                // link the last node to the head of 'dst'.
                // 'src' becomes the new head of 'dst'.
                auto d = dst.load(std::memory_order_relaxed);
                do {
                    n->next_.store(d, std::memory_order_relaxed);
                    // check if the head has changed and update it atomically.
                    // (if the CAS fails, 'd' is updated to the current 'dst' head)
                } while (!dst.compare_exchange_weak(d, src, std::memory_order_acq_rel)) ;
                return; // success
            } else {
                n = next;
            }
        }
    }

    void destroy_list(node *n){
        while (n){
            auto tmp = n;
            n = n->next_.load(std::memory_order_relaxed);
            tmp->~node();
            base::deallocate(tmp);
        }
    }
};

} // lockfree
} // aoo
