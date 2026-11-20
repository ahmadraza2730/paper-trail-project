// Separate-chaining hash map (Section 5.1).
// Rehash (double buckets) when size / buckets > 0.75 after an insert.
// New entries go at the END of their chain.
// Also used as an exact set: HashMap<K, bool>.
// Owner: Hassaan
#pragma once

#include <string>
#include "foundation/Hash.h"

template <typename K, typename V>
class HashMap {
public:
    explicit HashMap(int initialBuckets = 8);
    ~HashMap();
    HashMap(const HashMap& other);
    HashMap& operator=(const HashMap& other);

    void put(const K& key, const V& value);  // insert or overwrite
    V*   get(const K& key);                  // nullptr if missing
    const V* get(const K& key) const;
    bool contains(const K& key) const;
    bool remove(const K& key);               // false if missing
    int  size() const;
    void clear();

    // Visit every entry: fn(key, value). Order = bucket order, then chain order.
    template <typename Fn> void forEach(Fn fn) const;

private:
    struct Node { K key; V value; Node* next; };
    Node** buckets_;
    int    bucketCount_;
    int    size_;
};

// Uses hashKey() from Hash.h for both integer and string keys.
// TODO(Hassaan): implementation goes below.
