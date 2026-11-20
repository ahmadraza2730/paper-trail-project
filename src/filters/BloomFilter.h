// Plain Bloom filter (Sections 5.7.2, 5.10) and counting Bloom filter (Section 5.11).
// Positions use bloomPosition() from Hash.h.
// Owner: Hassaan
#pragma once

#include <cstdint>
#include <string>

class BloomFilter {
public:
    BloomFilter(uint64_t bits, int k);        // commit filters: 4 KB = 32768 bits, k = 3
    ~BloomFilter();
    BloomFilter(const BloomFilter& other);    // commits copy their parent's filter
    BloomFilter& operator=(const BloomFilter& other);

    void add(const std::string& item);
    bool mightContain(const std::string& item) const;
    void orWith(const BloomFilter& other);    // merge commits: OR of both parents

private:
    uint8_t* bits_;
    uint64_t m_;
    int      k_;
};

class CountingBloomFilter {
public:
    CountingBloomFilter(uint64_t counters = 1024, int k = 3);
    ~CountingBloomFilter();

    void add(const std::string& item);         // counters saturate at 15
    void remove(const std::string& item);      // a counter at 15 is never decremented
    bool mightContain(const std::string& item) const;
    int  saturatedCount() const;

private:
    uint8_t* counters_;
    uint64_t m_;
    int      k_;
};

// fptest bloom|counting <m> <k> <n>
void runFpTest(bool counting, uint64_t m, int k, int n);
