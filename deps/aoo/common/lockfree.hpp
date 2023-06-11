/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <stdint.h>
#include <atomic>
#include <vector>
#include <cassert>

#include "sync.hpp"

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
        std::allocator_traits<Alloc>::template rebind_alloc<typename C::node>
{
    typedef typename std::allocator_traits<Alloc>::template rebind_alloc<typename C::node> base;
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

// unbounded lockfree MPSC queue; multiple producers are synchronized with a simple spin lock.
// NB: the free list *could* be atomic, but we would need to be extra careful to avoid the
// ABA problem. (During a CAS loop the current node could be popped and pushed again, so that
// the CAS would succeed even though the object has changed.)
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
        first_ = divider_ = last_ = n;
    }

    unbounded_mpsc_queue(const unbounded_mpsc_queue&) = delete;

    unbounded_mpsc_queue(unbounded_mpsc_queue&& other)
        : base(std::move(other))
    {
        first_ = other.first_;
        divider_ = other.divider_;
        last_ = other.last_;
        other.first_ = nullptr;
        other.divider_ = nullptr;
        other.last_ = nullptr;
    }

    unbounded_mpsc_queue& operator=(unbounded_mpsc_queue&& other){
        base::operator=(std::move(other));
        first_ = other.first_;
        divider_ = other.divider_;
        last_ = other.last_;
        other.first_ = nullptr;
        other.divider_ = nullptr;
        other.last_ = nullptr;
        return *this;
    }

    ~unbounded_mpsc_queue() {
        auto it = first_;
        while (it){
            auto next = it->next_;
            it->~node();
            base::deallocate(it);
            it = next;
        }
    }

    // not thread-safe!
    void reserve(size_t n) {
        // check for existing empty nodes
        for (auto it = first_, end = divider_.load();
             it != end; it = it->next_) {
            n--;
        }
        // add empty nodes
        for (size_t i = 0; i < n; ++i) {
            auto tmp = base::allocate();
            new (tmp) node();
            tmp->next_ = first_;
            first_ = tmp;
        }
    }

    // can be called by several threads
    template<typename... U>
    void push(U&&... args) {
        auto n = get_node();
        n->data_ = T{std::forward<U>(args)...};
        push_node(n);
    }

    template<typename Fn>
    void produce(Fn&& func) {
        auto n = get_node();
        func(n->data_);
        push_node(n);
    }

    // must be called from a single thread!
    void pop(T& result){
        // use node *after* divider, because the divider itself is always a dummy!
        auto n = divider_.load(std::memory_order_relaxed)->next_;
        result = std::move(n->data_); // get the data
        divider_.store(n, std::memory_order_release); // publish new divider
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
        // use node *after* divider, because the divider itself is always a dummy!
        auto n = divider_.load(std::memory_order_relaxed)->next_;
        auto data = std::move(n->data_); // get the data
        divider_.store(n, std::memory_order_release); // publish new divider
        func(data); // finally use data
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
        return divider_.load(std::memory_order_relaxed)
                == last_.load(std::memory_order_relaxed);
    }

    // not thread-safe (?)
    void clear() {
        divider_.store(last_);
    }
 private:
    node * first_;
    std::atomic<node *> divider_;
    std::atomic<node *> last_;
    sync::spinlock lock_;

    node * get_node() {
        // try to reuse existing node
        sync::unique_lock<sync::spinlock> l(lock_);
        if (first_ != divider_.load(std::memory_order_relaxed)) {
            auto n = first_;
            first_ = first_->next_;
            n->next_ = nullptr; // !
            return n;
        } else {
            // allocate new node
            l.unlock();
            auto n = base::allocate();
            new (n) node{};
            return n;
        }
    }

    void push_node(node *n) {
        sync::scoped_lock<sync::spinlock> l(lock_);
        auto last = last_.load(std::memory_order_relaxed);
        last->next_ = n;
        last_.store(n, std::memory_order_release); // publish
    }
};

//------------------------ rcu_list ---------------------------------//

// A lock-free singly-linked list with RCU algorithm.
// It supports concurrent iteration and adding/removal of items,
// with a few restrictions:
// * you may only call methods while the list is locked; the only exception is update()
// * you may only access list items while the list is (still) locked
// * nodes must not be removed concurrently resp. without external synchronization

template<typename T, typename Alloc = std::allocator<T>>
class rcu_list :
    detail::node_allocator_base<detail::atomic_node_base<T>, Alloc>
{
    typedef detail::node_allocator_base<detail::atomic_node_base<T>, Alloc> base;
    typedef typename base::node node;
public:
    template<typename U>
    class base_iterator {
        friend class rcu_list;
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

    rcu_list(const Alloc& alloc = Alloc{})
        : base(alloc) {}

    rcu_list(const rcu_list&) = delete;

    rcu_list(rcu_list&& other)
        : base(std::move(other))
    {
        head_ = other.head_.exchange(nullptr);
        free_ = other.free_.exchange(nullptr);
        refcount_ = other.refcount_.exchange(0);
    }

    ~rcu_list(){
        destroy_list(head_.load());
        destroy_list(free_.load());
    }

    rcu_list& operator=(rcu_list&& other){
        base::operator=(std::move(other));
        head_ = other.head_.exchange(nullptr);
        free_ = other.free_.exchange(nullptr);
        refcount_ = other.refcount_.exchange(0);
        return *this;
    }

    // NB: can be called concurrently (while the list is locked)
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

    // NB: don't call concurrently!
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

    // NB: don't call concurrently!
    iterator erase(iterator it){
        for (;;){
            auto n = head_.load(std::memory_order_acquire);
            if (n == it.node_){
                // try to remove head
                // there is no ABA problem, see update().
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
                        // unlink the node
                        auto next2 = next->next_.load(std::memory_order_acquire);
                        n->next_.store(next2, std::memory_order_release);
                        dispose_node(next);
                        return iterator(next2);
                    }
                    n = next;
                }
                // reached end of list (shouldn't happen)
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
            dispose_list(head);
        }
    }

    void lock(){
        refcount_.fetch_add(1, std::memory_order_acquire);
    }

    void unlock(){
        refcount_.fetch_sub(1, std::memory_order_release);
    }

    // This method is called periodically from a non-RT thread to collect garbage.
    // Always call in unlocked state!
    // NB: items on the free list are never reused, so there is no ABA problem
    // in the CAS loop in erase(). We might put the items back again (see below),
    // but in this case the list head would still point to the original (unmodified) object.
    bool update(){
        // check if the list appears be non-empty; if yes, also check the refcount
        if (free_.load(std::memory_order_relaxed)
                && !refcount_.load(std::memory_order_relaxed)){
            // atomically unlink the whole freelist
            auto f = free_.exchange(nullptr);
            if (!f){
                return false; // shouldn't really happen...
            }
            // check the refcount again
        #if 1
            // use read-modify-write operation to prevent reordering in both directions
            int32_t expected = 0;
            if (refcount_.compare_exchange_strong(expected, 0, std::memory_order_acq_rel)) {
        #else
            if (!refcount_.load(std::memory_order_acquire)) {
        #endif
                // from this point the refcount may go up again, but it wouldn't
                // refer to our list items, so we can safely free the memory.
                destroy_list(f);
                return true;
            } else {
                // A reader aquired access in the meantime, so we put the items back
                // to the free list and try again later. If the free list is still empty,
                // we can simply atomically exchange the head pointer.
                node *expected = nullptr;
                if (!free_.compare_exchange_strong(expected, f)){
                    // otherwise prepend the old free list to the new one
                    dispose_list(f);
                }
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

    void dispose_list(node *list){
        // get last node in list
        auto tail = list;
        for (;;){
            auto next = tail->next_.load(std::memory_order_relaxed);
            if (next) {
                tail = next;
            } else {
                break;
            }
        }
        // prepend to the free list; 'list' becomes new head
        // NB: there is no ABA problem because this method is only called
        // from update(), so nobody can concurrently *remove* items.
        // (It is possible that new items are pushed by erase(), though.)
        auto head = free_.load(std::memory_order_relaxed);
        do {
            tail->next_.store(head, std::memory_order_relaxed);
            // check if the head has changed and update it atomically.
            // (if the CAS fails, 'head' is updated to the actual list head)
        } while (!free_.compare_exchange_weak(head, list, std::memory_order_acq_rel)) ;
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
