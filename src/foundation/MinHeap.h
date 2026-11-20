// Bounded min-heap that keeps the k largest items (Section 5.1, used by "similar").
// Less(a, b) returns true if a is "smaller" than b.
// Owner: Hassaan
#pragma once

#include "foundation/DynArray.h"

template <typename T, typename Less>
class MinHeap {
public:
    MinHeap(int capacity, Less less);

    void offer(const T& item);   // push if not full; else replace root only if strictly larger
    T    popMin();
    const T& peekMin() const;
    int  size() const;
    bool empty() const;

private:
    DynArray<T> items_;
    int  capacity_;
    Less less_;
};

// TODO(Hassaan): implementation goes below.
