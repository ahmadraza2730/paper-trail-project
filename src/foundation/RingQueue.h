#pragma once

template <typename T>
class RingQueue {
private:
    T* data;
    int front;
    int count;
    int capacity;

    void grow() {
        int newCapacity = capacity * 2;
        T* newData = new T[newCapacity];
        for (int i = 0; i < count; i++) {
            newData[i] = data[(front + i) % capacity];
        }
        delete[] data;
        data = newData;
        front = 0;
        capacity = newCapacity;
    }

public:
    RingQueue() {
        capacity = 4;
        front = 0;
        count = 0;
        data = new T[capacity];
    }

    ~RingQueue() {
        delete[] data;
    }

    RingQueue(const RingQueue& other) {
        capacity = other.capacity;
        count = other.count;
        front = 0;
        data = new T[capacity];
        for (int i = 0; i < count; i++) {
            data[i] = other.data[(other.front + i) % other.capacity];
        }
    }

    RingQueue& operator=(const RingQueue& other) {
        if (this == &other) {
            return *this;
        }
        delete[] data;
        capacity = other.capacity;
        count = other.count;
        front = 0;
        data = new T[capacity];
        for (int i = 0; i < count; i++) {
            data[i] = other.data[(other.front + i) % other.capacity];
        }
        return *this;
    }

    void add(const T& value) {
        if (count == capacity) {
            grow();
        }
        data[(front + count) % capacity] = value;
        count++;
    }

    T remove() {
        T value = data[front];
        front = (front + 1) % capacity;
        count--;
        return value;
    }

    T& peek() {
        return data[front];
    }

    int size() const {
        return count;
    }

    bool isEmpty() const {
        return count == 0;
    }

    void clear() {
        front = 0;
        count = 0;
    }
};
