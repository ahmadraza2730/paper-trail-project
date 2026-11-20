// Global B+ tree inverted index: token -> sorted posting list (Section 5.7.1).
// Owner: Hassaan
#pragma once

#include <string>
#include "common/Types.h"
#include "foundation/DynArray.h"

#ifndef BPTREE_ORDER
#define BPTREE_ORDER 4      // compile-time order m >= 3 (fuzz runs use 3)
#endif

struct Posting {
    FileID file;
    LineID line;
};

class BPlusTree {
public:
    BPlusTree();
    ~BPlusTree();

    void addPosting   (const std::string& token, FileID file, LineID line);
    void removePosting(const std::string& token, FileID file, LineID line);  // removes key if list empties
    const DynArray<Posting>* find(const std::string& token) const;          // nullptr if absent

    // Range scan along leaf links for every key starting with prefix: fn(key, postings).
    template <typename Fn> void forEachWithPrefix(const std::string& prefix, Fn fn) const;

    void copyPostings (FileID from, FileID to);  // fork
    void purgeFile    (FileID file);             // emptybin

private:
    struct BNode;
    BNode* root_;
};
