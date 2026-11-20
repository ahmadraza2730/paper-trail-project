// Hash functions fixed by the spec (Section 3.3), so hand traces are reproducible.
// Owner: Hassaan
#pragma once

#include <cstdint>
#include <string>

uint32_t hashInt(uint64_t id);                 // (id * 2654435761) mod 2^32
uint32_t fnv1a(const std::string& s);          // FNV-1a 32-bit
uint32_t djb2(const std::string& s);           // start 5381, h = h*33 + byte

// Bloom filter position i: (h1 + i*h2) mod m, with h1 = fnv1a, h2 = djb2 | 1, 64-bit math.
uint64_t bloomPosition(const std::string& s, int i, uint64_t m);

// Overloads used by HashMap.
inline uint32_t hashKey(uint64_t key)           { return hashInt(key); }
inline uint32_t hashKey(uint32_t key)           { return hashInt(key); }
inline uint32_t hashKey(const std::string& key) { return fnv1a(key); }
