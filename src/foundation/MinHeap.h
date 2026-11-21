#pragma once

#include "foundation/DynArray.h"

template <typename T>
class MinHeap {
private:
    DynArray<T> items;

    void swapItems(int a, int b) {
        T temp = items.get(a);
        items.set(a, items.get(b));
        items.set(b, temp);
    }

    void moveUp(int index) {
        while (index > 1) {
            int parent = index / 2;
            if (items.get(index) < items.get(parent)) {
                swapItems(index, parent);
                index = parent;
            } else {
                return;
            }
        }
    }

    void moveDown(int index) {
        while (true) {
            int left = index * 2;
            int right = index * 2 + 1;
            int smallest = index;

            //if left child exist and is smaller than smallest(parent), 
            // left index is now smaller
            if (left <= size() && items.get(left) < items.get(smallest)) {
                smallest = left;
            }

            //if right child exist and is smaller than smallest, 
            // then right index is now smallest
            if (right <= size() && items.get(right) < items.get(smallest)) {
                smallest = right;
            }

            //if smallest hasn't changed after previous 2 ifs, 
            // it means that parent is smaller
            // or has no children
            //no need to swap
            if (smallest == index) {
                return;
            }

            //parent is larger, so swap the parent with the smaller of children
            swapItems(index, smallest);
            //index now moves down for next loop
            index = smallest;
        }
    }

public:
    MinHeap() {
        items.add(T());
    }

    void add(const T& value) {
        items.add(value);
        moveUp(size());
    }

    T removeMin() {
        T smallest = items.get(1);
        items.set(1, items.get(size()));
        items.remove();
        moveDown(1);
        return smallest;
    }

    T& peekMin() {
        return items.get(1);
    }

    int size() const {
        return items.size() - 1;
    }

    bool isEmpty() const {
        return size() == 0;
    }

    void clear() {
        items.clear();
        items.add(T());
    }
};
