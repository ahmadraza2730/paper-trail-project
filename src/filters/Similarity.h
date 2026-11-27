#pragma once

#include <string>
#include "foundation/DynArray.h"
#include "foundation/HashMap.h"
#include "filters/BloomFilter.h"

// Similarity checker (spec Section 5.10): `similar <k> [percent]`.
//
// How it fits together:
//   1. For each document, tokenise every line and join the tokens in line order.
//   2. Every run of 3 consecutive tokens is a shingle ("eggs and flour").
//      Keep the distinct ones; n is how many there are.
//   3. Put the shingles in a Bloom filter (m = 10n bits, k = 7 hashes) and in
//      an exact hash set.
//   4. For every pair (A, B), containment of A in B = how many of A's shingles
//      B reports present / n(A). The pair's score is the larger of the two
//      directions. The Bloom filter gives the estimate, the hash set the exact value.
//   5. A min-heap of size k keeps the k best pairs (by the Bloom estimate).

// Asks the history part whether two documents share a common ancestor commit.
// Returns true and sets commitName (for example "c2") when they do.
typedef bool (*CommonAncestorFn)(int fileIdA, int fileIdB, std::string& commitName, void* context);

// One document prepared for comparison.
struct SimilarityDoc {
    int fileId;
    std::string name;
    DynArray<std::string> shingles;     // distinct shingles, in first-seen order
    HashMap<std::string, bool>* exact;  // exact set of the same shingles
    BloomFilter* filter;                // m = 10n bits, k = 7
};

// A score kept as a fraction (hits / total) so comparisons are exact.
struct Score {
    int hits;
    int total;
};

// One compared pair. a and b index into the checker's document list.
struct SimilarPair {
    int a;
    int b;
    Score bloom;   // Bloom estimate (the larger direction)
    Score exact;   // exact value (the larger direction)
    int visit;     // visit number, used to break ties

    // Heap order: lower score is smaller; with equal scores the pair visited
    // later counts as smaller, so it is the one pushed out first.
    bool operator<(const SimilarPair& other) const;
};

// Distinct shingles (w = 3 tokens) of a document, in first-seen order.
DynArray<std::string> makeShingles(const DynArray<std::string>& lines);

// Whole percent, halves rounded up. percentOf({1, 8}) = 13.
int percentOf(const Score& s);

// Keeps the k best pairs with a bounded min-heap and returns them best first.
DynArray<SimilarPair> topPairs(const DynArray<SimilarPair>& candidates, int k);

// Reads "similar <k> [percent]". words[0] is "similar".
// Prints an error and returns false if a number is not valid.
bool parseSimilarArgs(const DynArray<std::string>& words, int& k, int& percent);

class SimilarityChecker {
private:
    DynArray<SimilarityDoc*> docs;   // only documents with at least one shingle

    void sortByFileId();

public:
    SimilarityChecker();
    ~SimilarityChecker();
    SimilarityChecker(const SimilarityChecker&) = delete;
    SimilarityChecker& operator=(const SimilarityChecker&) = delete;

    // Adds one live, non-empty document with its current working text.
    // A document with fewer than 3 tokens has no shingles and is skipped.
    void addDocument(int fileId, const std::string& name, const DynArray<std::string>& lines);

    int documentCount() const;

    // Compares documents number a and b (in the order they were added,
    // or FileID order once report has run). Used by report and by the tests.
    SimilarPair comparePair(int a, int b, int visit) const;

    // Prints the report for `similar <k> <percent>`.
    // lca may be nullptr while the commit history is not connected yet.
    void report(int k, int percent, CommonAncestorFn lca, void* context);
};
