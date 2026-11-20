// LIFO stack built on DynArray (Section 5.1). Used by DFS, commit replay, merge.
// Owner: Hassaan
#pragma once

#include "foundation/DynArray.h"

template <typename T>
class Stack {
public:
    void push(const T& value);
    T    pop();
    T&   top();
    int  size() const;
    bool empty() const;
    void clear();

private:
    DynArray<T> items_;
};

// TODO(Hassaan): implementation goes below.
