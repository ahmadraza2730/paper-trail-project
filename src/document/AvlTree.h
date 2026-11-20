// Order-statistic AVL tree: in-order sequence = line order (Section 5.3.1).
// Owner: Huzaima
#pragma once

#include <string>
#include "common/Types.h"

struct LineNode {
    LineID      id;
    std::string text;
    std::string author;
    LineNode*   left;
    LineNode*   right;
    LineNode*   parent;
    int         height;
    int         size;       // nodes in this subtree
};

class AvlTree {
public:
    AvlTree();
    ~AvlTree();
    AvlTree(const AvlTree& other);            // deep copy (fork, merge working copy)
    AvlTree& operator=(const AvlTree& other);

    // anchor == nullptr means insert at the top. O(log N).
    LineNode* insertAfter(LineNode* anchor, LineID id,
                          const std::string& text, const std::string& author);
    void      erase(LineNode* node);          // O(log N)
    LineNode* select(int k) const;            // 1-based line k, O(log N)
    int       rank(const LineNode* node) const;  // 1-based line number, O(log N)
    LineNode* last() const;
    int       size() const;

    template <typename Fn> void forEachInOrder(Fn fn) const;

private:
    LineNode* root_;
};
