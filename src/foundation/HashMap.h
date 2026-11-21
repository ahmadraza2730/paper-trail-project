#pragma once

#include <string>
#include "foundation/DynArray.h"
#include "foundation/Hash.h"

template <typename K, typename V>
class HashMap {
private:
    struct Node {
        K key;
        V value;
        Node* next;
    };

    Node** buckets;
    int capacity;
    int count;

    static unsigned int hashOf(const std::string& key) {
        return fnv1a(key);
    }

    static unsigned int hashOf(int key) {
        return hashInt((unsigned int)key);
    }

    //gives slot
    int slotOf(const K& key) const {
        return hashOf(key) % capacity;
    }

    //finds a node by going to the slot
    //and walking through the linked list

    Node* findNode(const K& key) const {
        Node* current = buckets[slotOf(key)];
        while (current != nullptr) {
            if (current->key == key) {
                return current;
            }
            current = current->next;
        }
        return nullptr;
    }

    //make an array of new capacity
    //every slot is empty
    void makeEmpty(int newCapacity) {
        capacity = newCapacity;
        count = 0;
        buckets = new Node*[capacity];
        for (int i = 0; i < capacity; i++) {
            buckets[i] = nullptr;
        }
    }

    //delete each linked list
    //then delete whole array
    void freeAll() {
        for (int i = 0; i < capacity; i++) {
            Node* current = buckets[i];
            while (current != nullptr) {
                Node* next = current->next;
                delete current;
                current = next;
            }
        }
        delete[] buckets;
    }

    //copy from another hashmap
    void copyFrom(const HashMap& other) {
        makeEmpty(other.capacity);
        for (int i = 0; i < other.capacity; i++) {
            Node* current = other.buckets[i];
            while (current != nullptr) {
                put(current->key, current->value);
                current = current->next;
            }
        }
    }


    void grow() {
        Node** oldBuckets = buckets;
        int oldCapacity = capacity;
        makeEmpty(oldCapacity * 2);
        for (int i = 0; i < oldCapacity; i++) {
            Node* current = oldBuckets[i];
            while (current != nullptr) {
                Node* next = current->next;
                int slot = slotOf(current->key);
                current->next = buckets[slot];
                buckets[slot] = current;
                count++;
                current = next;
            }
        }
        delete[] oldBuckets;
    }

public:
    HashMap() {
        makeEmpty(8);
    }

    ~HashMap() {
        freeAll();
    }

    HashMap(const HashMap& other) {
        copyFrom(other);
    }

    HashMap& operator=(const HashMap& other) {
        if (this == &other) {
            return *this;
        }
        freeAll();
        copyFrom(other);
        return *this;
    }

    void put(const K& key, const V& value) {
        //updates the value
        Node* existing = findNode(key);
        if (existing != nullptr) {
            existing->value = value;
            return;
        }
        //put the new node at the front of the linked list
        int slot = slotOf(key);
        Node* node = new Node;
        node->key = key;
        node->value = value;
        node->next = buckets[slot];
        buckets[slot] = node;
        count++;
        if (count * 4 > capacity * 3) {
            grow();
        }
    }

    bool contains(const K& key) const {
        return findNode(key) != nullptr;
    }

    V& get(const K& key) {
        return findNode(key)->value;
    }

    bool remove(const K& key) {
        int slot = slotOf(key);
        Node* current = buckets[slot];
        Node* previous = nullptr;
        while (current != nullptr) {
            if (current->key == key) {
                if (previous == nullptr) {
                    buckets[slot] = current->next;
                } else {
                    previous->next = current->next;
                }
                delete current;
                count--;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

    int size() const {
        return count;
    }

    bool isEmpty() const {
        return count == 0;
    }

    DynArray<K> keys() const {
        DynArray<K> result;
        for (int i = 0; i < capacity; i++) {
            Node* current = buckets[i];
            while (current != nullptr) {
                result.add(current->key);
                current = current->next;
            }
        }
        return result;
    }
};
