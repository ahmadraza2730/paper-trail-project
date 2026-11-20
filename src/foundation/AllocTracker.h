// Allocation tracker (Section 5.12).
// Global operator new / delete (all forms) are overridden in AllocTracker.cpp,
// so every allocation, including std::string's, is counted.
// Owner: Hassaan
#pragma once

#include <cstdint>

uint64_t allocationsMade();
uint64_t allocationsFreed();

// Prints "allocations: X made, Y freed, Z live", or "LEAK: Z live allocations".
// Returns true if there is no leak.
bool printAllocationReport();
