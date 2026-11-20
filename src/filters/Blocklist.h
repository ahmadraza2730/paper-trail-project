// Blocked words: counting Bloom filter in front of an exact set (Section 5.11).
// Owner: Hassaan
#pragma once

#include <string>
#include "filters/BloomFilter.h"
#include "foundation/HashMap.h"

class Blocklist {
public:
    Blocklist();

    void block(const std::string& word);
    void unblock(const std::string& word);
    void listBlocked() const;                      // alphabetical + "exact lookups avoided: N"

    bool isBlocked(const std::string& token);      // counts avoided lookups
    // Prints a warning for each blocked token in the line (line number is 1-based).
    void warnLine(const std::string& text, int lineNumber);
    bool lineHasBlocked(const std::string& text);  // for the status line count

private:
    CountingBloomFilter      filter_;
    HashMap<std::string, bool> exact_;
    long long                avoided_;
};
