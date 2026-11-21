#pragma once

template <typename T>
class DynArray {
private:
    T* data;
    int count;
    int capacity;

    void grow() {
        int newCapacity = capacity * 2;
        T* newData = new T[newCapacity];
        for (int i = 0; i < count; i++) {
            newData[i] = data[i];
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

public:
    DynArray() {
        capacity = 4;
        count = 0;
        data = new T[capacity];
    }

    ~DynArray() {
        delete[] data;
    }

    DynArray(const DynArray& other) {
        capacity = other.capacity;
        count = other.count;
        data = new T[capacity];
        for (int i = 0; i < count; i++) {
            data[i] = other.data[i];
        }
    }

    DynArray& operator=(const DynArray& other) {
        if (this == &other) {
            return *this;
        }
        delete[] data;
        capacity = other.capacity;
        count = other.count;
        data = new T[capacity];
        for (int i = 0; i < count; i++) {
            data[i] = other.data[i];
        }
        return *this;
    }

    void add(const T& value) {
        if (count == capacity) {
            grow();
        }
        data[count] = value;
        count++;
    }

    void remove() {
        if (count > 0) {
            count--;
        }
    }

    T& get(int index) {
        return data[index];
    }

    const T& get(int index) const {
        return data[index];
    }

    void set(int index, const T& value) {
        data[index] = value;
    }

    int size() const {
        return count;
    }

    bool isEmpty() const {
        return count == 0;
    }

    void clear() {
        count = 0;
    }
};
