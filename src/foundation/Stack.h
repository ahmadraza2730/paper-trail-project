#pragma once

#include "foundation/DynArray.h"

template <typename T>
class Stack {
private:
    DynArray<T> items;

public:
    void push(const T& value) {
        items.add(value);
    }

    T pop() {
        T value = items.get(items.size() - 1);
        items.remove();
        return value;
    }

    T& top() {
        return items.get(items.size() - 1);
    }

    int size() const {
        return items.size();
    }

    bool isEmpty() const {
        return items.isEmpty();
    }

    void clear() {
        items.clear();
    }
};
