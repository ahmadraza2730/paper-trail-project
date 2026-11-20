// Stable merge sort (Section 5.1). Equal items keep their previous order.
// For descending order pass a reversed comparison, never reverse the list.
// Owner: Hassaan
#pragma once

#include <string>

// less(a, b) returns true if a must come before b.
template <typename T, typename Less>
void stableSort(T* items, int count, Less less);

// Case-insensitive name comparison used by ls and blocked.
bool lessIgnoreCase(const std::string& a, const std::string& b);

// TODO(Hassaan): template implementation goes below.
