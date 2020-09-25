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

/*////////////////////// queue /////////////////////////*/

// a lock-free queue which supports reading/writing data
// in fixed-sized blocks.
template<typename T>
class queue {
 public:
    queue() = default;
    // we need a move constructor so we can
    // put it in STL containers
    queue(queue&& other)
        : balance_(other.balance_.load()),
          rdhead_(other.rdhead_),
          wrhead_(other.wrhead_),
          stride_(other.stride_),
          data_(std::move(other.data_))
    {}
    queue& operator=(queue&& other){
        balance_ = other.balance_.load();
        rdhead_ = other.rdhead_;
        wrhead_ = other.wrhead_;
        stride_ = other.stride_;
        data_ = std::move(other.data_);
        return *this;
    }

    void resize(int32_t size, int32_t blocksize) {
        // check if size is divisible by both rdsize and wrsize
        assert(size >= blocksize);
        assert((size % blocksize) == 0);
    #if 1
        data_.clear(); // force zero
    #endif
        data_.resize(size);
        stride_ = blocksize;
        reset();
    }

    int32_t blocksize() const { return stride_; }

    int32_t capacity() const { return data_.size(); }

    void reset() {
        rdhead_ = wrhead_ = 0;
        balance_ = 0;
    }
    // returns: the number of available *blocks* for reading
    int32_t read_available() const {
        if (stride_){
            return balance_.load(std::memory_order_acquire) / stride_;
        } else {
            return 0;
        }
    }

    void read(T& out) {
        out = std::move(data_[rdhead_]);
        rdhead_ = (rdhead_ + 1) % capacity();
        --balance_;
        assert(balance_ >= 0);
    }

    const T* read_data() const {
        return &data_[rdhead_];
    }

    void read_commit() {
        rdhead_ = (rdhead_ + stride_) % capacity();
        balance_ -= stride_;
        assert(balance_ >= 0);
    }
    // returns: the number of available *blocks* for writing
    int32_t write_available() const {
        if (stride_){
            return (capacity() - balance_.load(std::memory_order_acquire)) / stride_;
        } else {
            return 0;
        }
    }

    template<typename U>
    void write(U&& value) {
        data_[wrhead_] = std::forward<U>(value);
        wrhead_ = (wrhead_ + 1) % capacity();
        ++balance_;
        assert(balance_ <= capacity());
    }

    T* write_data() {
        return &data_[wrhead_];
    }

    void write_commit() {
        wrhead_ = (wrhead_ + stride_) % capacity();
        balance_ += stride_;
        assert(balance_ <= capacity());
    }
 private:
    std::atomic<int32_t> balance_{0};
    int32_t rdhead_{0};
    int32_t wrhead_{0};
    int32_t stride_{0};
    std::vector<T> data_;
};

/*///////////////////////// list ////////////////////////*/

// a lock-free singly-linked list which supports adding items and iteration.
// clearing the list is *not* thread-safe

template<typename T>
class list {
public:
    struct node {
        node* next_;
        T data_;
        template<typename... U>
        node(U&&... args)
            : next_(nullptr), data_(std::forward<U>(args)...) {}
    };

    template<typename U>
    class base_iterator {
        U *node_;
    public:
        base_iterator()
            : node_(nullptr){}
        base_iterator(U *n)
            : node_(n){}
        base_iterator(const base_iterator&) = default;
        base_iterator& operator=(const base_iterator&) = default;
        T& operator*() { return node_->data_; }
        T* operator->() { return &node_->data; }
        base_iterator& operator++() {
            node_ = node_->next_;
            return *this;
        }
        base_iterator operator++(int) {
            base_iterator old = *this;
            node_ = node_->next_;
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

    list()
        : head_(nullptr){}
    list(const list&) = delete;
    list(list&& other){
        // not sure...
        auto head = other.head_.exchange(nullptr);
        head_.store(head);
    }
    list& operator=(list&& other){
        // not sure...
        auto head = other.head_.exchange(nullptr);
        head_.store(head);
        return *this;
    }

    template<typename... U>
    void emplace_front(U&&... args){
        auto n = new node(std::forward<U>(args)...);
        while (true){
            auto head = head_.load(std::memory_order_acquire);
            n->next_ = head;
            // check if the head has changed and update it atomically
            if (head_.compare_exchange_strong(head, n)){
                break;
            }
        }
        size_++;
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

    int32_t size() const { return size_.load(std::memory_order_acquire); }

    // the deletion of nodes itself is not thread-safe!!!
    void clear(){
        size_ = 0;
        auto it = head_.exchange(nullptr);
        while (it){
            auto next = it->next_;
            delete it;
            it = next;
        }
    }
    ~list(){
        clear();
    }
private:
    std::atomic<node *> head_{nullptr};
    std::atomic<int32_t> size_{0};
};

} // lockfree
} // aoo
