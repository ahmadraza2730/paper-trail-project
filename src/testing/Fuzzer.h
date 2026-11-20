// Seeded fuzzer (Section 5.12): "fuzz <seed> <count>".
// Generator: state = (state * 1103515245 + 12345) mod 2^31.
// Owner: framework Hassaan, command generators by each owner
#pragma once

#include <cstdint>
#include <string>

class Rng {
public:
    explicit Rng(uint32_t seed);
    uint32_t next();
    int      below(int n);        // 0 .. n-1
private:
    uint32_t state_;
};

// Runs count random valid commands with the checker on, writing them to fuzz_<seed>.txt.
void runFuzz(uint32_t seed, int count);
