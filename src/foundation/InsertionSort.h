#pragma once

#include "foundation/DynArray.h"

template <typename T>
void insertionSort(DynArray<T>& items, bool (*comesBefore)(const T&, const T&)) {
    for (int i = 1; i < items.size(); i++) {
        T current = items.get(i);
        int j = i - 1;
        while (j >= 0 && comesBefore(current, items.get(j))) {
            items.set(j + 1, items.get(j));
            j--;
        }
        items.set(j + 1, current);
    }
}
