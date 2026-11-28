#pragma once

#include <string>
#include "foundation/DynArray.h"
#include "search/Search.h"

// B+ tree word index (spec Section 5.7.1).
//
// Maps every token to its posting list: the sorted (FileID, LineID) pairs of
// the saved lines that contain it. Search uses it through the WordIndex
// interface in Search.h, so nothing in search depends on how the tree works.
//
// Layout:
//   - Internal nodes hold only separator keys and child pointers. They guide
//     the descent: child i holds keys < keys[i], child i+1 holds keys >= keys[i].
//   - Every token and its posting list lives in a leaf.
//   - The leaves are linked left to right in key order, so a prefix scan is
//     one descent followed by a walk along the leaves.
//   - All leaves are at the same depth, and every node except the root is at
//     least about half full, so find, insert and remove are O(log n).

// Max children of an internal node. A small order makes splits and merges
// happen often, which is what we want while testing; it can be raised freely.
const int BPLUS_ORDER = 4;
// Max keys in any node.
const int BPLUS_MAX_KEYS = BPLUS_ORDER - 1;
// Min keys in a leaf that is not the root: ceil((order - 1) / 2).
const int BPLUS_MIN_LEAF_KEYS = BPLUS_ORDER / 2;
// Min keys in an internal node that is not the root: ceil(order / 2) - 1.
const int BPLUS_MIN_INTERNAL_KEYS = (BPLUS_ORDER + 1) / 2 - 1;

struct BNode {
    bool leaf;
    int count;  // keys in use
    // One slot more than allowed, so a node can overflow for a moment before it splits.
    std::string keys[BPLUS_ORDER];
    BNode* children[BPLUS_ORDER + 1];               // internal nodes only
    DynArray<SearchPosting>* lists[BPLUS_ORDER];    // leaves only: lists[i] belongs to keys[i]
    BNode* next;                                    // leaves only: the next leaf to the right
};

class BPlusTree : public WordIndex {
private:
    BNode* root;
    int keys;  // number of distinct tokens

    // nodes
    BNode* newNode(bool leaf);
    void freeNode(BNode* node);
    int childIndex(const BNode* node, const std::string& token) const;
    BNode* findLeaf(const std::string& token) const;
    DynArray<SearchPosting>* lookup(const std::string& token) const;

    // insert
    bool insertInto(BNode* node, const std::string& token, DynArray<SearchPosting>* list,
                    std::string& upKey, BNode*& rightHalf);
    void splitLeaf(BNode* leaf, std::string& upKey, BNode*& rightHalf);
    void splitInternal(BNode* node, std::string& upKey, BNode*& rightHalf);

    // remove
    void removeFrom(BNode* node, const std::string& token);
    bool underflow(const BNode* node) const;
    bool canLend(const BNode* node) const;
    void fixChild(BNode* parent, int i);
    void borrowFromLeft(BNode* parent, int i);
    void borrowFromRight(BNode* parent, int i);
    void mergeChildren(BNode* parent, int i);

    // checks
    bool checkNode(const BNode* node, int depth, int& leafDepth, const std::string* low,
                   const std::string* high, std::string& why) const;
    void describeLevel(const BNode* node, int level, int target, std::string& out) const;

public:
    BPlusTree();
    ~BPlusTree();
    BPlusTree(const BPlusTree&) = delete;
    BPlusTree& operator=(const BPlusTree&) = delete;

    // WordIndex
    void addPosting(const std::string& token, int file, int line) override;
    void removePosting(const std::string& token, int file, int line) override;
    const DynArray<SearchPosting>* find(const std::string& token) const override;
    void scanPrefix(const std::string& prefix, PostingListFn fn, void* context) const override;

    // for tests and the demo
    int keyCount() const;
    int height() const;
    // Checks every B+ tree rule. On a break, returns false and says why.
    bool validate(std::string& why) const;
    // The tree level by level, one line per level, e.g.
    //   [honey]
    //   [apple bread] [honey milk]
    std::string describe() const;
};
