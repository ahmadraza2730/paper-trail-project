#pragma once

#include <string>
#include "foundation/DynArray.h"
#include "foundation/HashMap.h"

class Blocklist {
private:
    DynArray<unsigned char> counters;
    int m;
    int k;
    int avoided;
    HashMap<std::string, bool> words;

    int position(const std::string& word, int i) const;
    bool cleanWord(const std::string& word, std::string& result) const;
    bool uncertain(const std::string& word) const;

public:
    Blocklist(int counterCount = 1024, int hashCount = 3);

    void block(const std::string& word);
    void unblock(const std::string& word);

    bool isBlocked(const std::string& word);
    bool checkLine(int lineNumber, const std::string& text);
    bool lineHasBlocked(const std::string& text) const;
    void printBlocked() const;
    void runFpTest(int n);
};
