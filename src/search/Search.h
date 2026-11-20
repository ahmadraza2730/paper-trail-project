// "search <word>" and "search <prefix>*" (Section 5.7.2).
// Owner: Ahmad
#pragma once

#include <string>
#include "engine/Engine.h"
#include "search/BPlusTree.h"

class SearchIndex {
public:
    BPlusTree tree;

    // Update from saved ops (called through onOpsSaved).
    void applySavedOps(FileID file, const Op* ops, int count);
};

void runSearch(Engine& e, const std::string& term);
