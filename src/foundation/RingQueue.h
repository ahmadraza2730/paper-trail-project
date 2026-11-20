// Growable, indexable circular buffer (Section 5.1).
// Used for staging queues, commit op queues and BFS.
// Owner: Hassaan
#pragma once

template <typename T>
class RingQueue {
public:
    RingQueue();
    ~RingQueue();
    RingQueue(const RingQueue& other);
    RingQueue& operator=(const RingQueue& other);

    void push(const T& value);       // add at back; doubles when full, never overwrites
    T    pop();                      // remove from front
    T&       get(int i);             // i-th item from the front
    const T& get(int i) const;
    int  size() const;
    bool empty() const;
    void clear();

private:
    T*  data_;
    int head_;
    int size_;
    int capacity_;
};

// TODO(Hassaan): implementation goes below.
