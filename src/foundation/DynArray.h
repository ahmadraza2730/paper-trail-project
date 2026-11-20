// Growable array, doubles when full (Section 5.1).
// Owner: Hassaan
#pragma once

template <typename T>
class DynArray {
public:
    DynArray();
    ~DynArray();
    DynArray(const DynArray& other);
    DynArray& operator=(const DynArray& other);

    void push(const T& value);       // O(1) amortised
    void pop();                      // removes last item
    void removeAt(int index);        // shifts later items left, keeps order
    T&       operator[](int index);
    const T& operator[](int index) const;
    T&   back();
    int  size() const;
    bool empty() const;
    void clear();

private:
    T*  data_;
    int size_;
    int capacity_;
};

// TODO(Hassaan): implementation goes below (templates must live in the header).
