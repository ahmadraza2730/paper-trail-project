#include "search/BPlusTree.h"

using namespace std;

// ---------- posting lists ----------

// true if posting p comes before (file, line): sorted by file, then line
static bool before(const SearchPosting& p, int file, int line) {
    if (p.file != file) {
        return p.file < file;
    }
    return p.line < line;
}

// binary search: index of the first posting that is not before (file, line)
static int lowerBound(const DynArray<SearchPosting>& list, int file, int line) {
    int lo = 0;
    int hi = list.size();
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (before(list.get(mid), file, line)) {
            lo = mid + 1;
        }
        else {
            hi = mid;
        }
    }
    return lo;
}

static bool samePosting(const SearchPosting& p, int file, int line) {
    return p.file == file && p.line == line;
}

// ---------- nodes ----------

BNode* BPlusTree::newNode(bool leaf) {
    BNode* node = new BNode;
    node->leaf = leaf;
    node->count = 0;
    node->next = nullptr;
    for (int i = 0; i < BPLUS_ORDER; i++) {
        node->lists[i] = nullptr;
    }
    for (int i = 0; i <= BPLUS_ORDER; i++) {
        node->children[i] = nullptr;
    }
    return node;
}

// frees a whole subtree, with the posting lists in its leaves
void BPlusTree::freeNode(BNode* node) {
    if (node->leaf) {
        for (int i = 0; i < node->count; i++) {
            delete node->lists[i];
        }
    }
    else {
        for (int i = 0; i <= node->count; i++) {
            freeNode(node->children[i]);
        }
    }
    delete node;
}

// which child of an internal node the token belongs under:
// child i holds keys < keys[i], so it is the first i with token < keys[i]
// (nodes hold at most BPLUS_MAX_KEYS keys, so a linear scan is enough)
int BPlusTree::childIndex(const BNode* node, const string& token) const {
    int i = 0;
    while (i < node->count && !(token < node->keys[i])) {
        i++;
    }
    return i;
}

// walks from the root down to the leaf where the token is, or would be
BNode* BPlusTree::findLeaf(const string& token) const {
    BNode* node = root;
    while (!node->leaf) {
        node = node->children[childIndex(node, token)];
    }
    return node;
}

BPlusTree::BPlusTree() {
    root = newNode(true);  // an empty tree is one empty leaf
    keys = 0;
}

BPlusTree::~BPlusTree() {
    freeNode(root);
}

// ---------- find ----------

// the posting list of exactly this token, or nullptr
DynArray<SearchPosting>* BPlusTree::lookup(const string& token) const {
    BNode* leaf = findLeaf(token);
    for (int i = 0; i < leaf->count; i++) {
        if (leaf->keys[i] == token) {
            return leaf->lists[i];
        }
    }
    return nullptr;
}

const DynArray<SearchPosting>* BPlusTree::find(const string& token) const {
    return lookup(token);
}

// one descent to the first key >= prefix, then along the leaf chain
// until a key no longer starts with the prefix
void BPlusTree::scanPrefix(const string& prefix, PostingListFn fn, void* context) const {
    BNode* leaf = findLeaf(prefix);
    while (leaf != nullptr) {
        for (int i = 0; i < leaf->count; i++) {
            const string& key = leaf->keys[i];
            if (key < prefix) {
                continue;
            }
            if (key.compare(0, prefix.length(), prefix) != 0) {
                return;  // keys are sorted, so nothing further can match
            }
            fn(*leaf->lists[i], context);
        }
        leaf = leaf->next;
    }
}

// ---------- insert ----------

void BPlusTree::addPosting(const string& token, int file, int line) {
    //the token is already a key: add the pair to its list, keeping it sorted
    DynArray<SearchPosting>* existing = lookup(token);
    if (existing != nullptr) {
        int pos = lowerBound(*existing, file, line);
        if (pos < existing->size() && samePosting(existing->get(pos), file, line)) {
            return;  // already there
        }
        SearchPosting p;
        p.file = file;
        p.line = line;
        existing->add(p);  // grow by one, then shift the tail right
        for (int i = existing->size() - 1; i > pos; i--) {
            existing->set(i, existing->get(i - 1));
        }
        existing->set(pos, p);
        return;
    }

    //a new token: make its list and insert the key
    DynArray<SearchPosting>* list = new DynArray<SearchPosting>();
    SearchPosting p;
    p.file = file;
    p.line = line;
    list->add(p);

    string upKey;
    BNode* rightHalf = nullptr;
    if (insertInto(root, token, list, upKey, rightHalf)) {
        //the root split: a new root on top, the only way the tree gets taller
        BNode* newRoot = newNode(false);
        newRoot->count = 1;
        newRoot->keys[0] = upKey;
        newRoot->children[0] = root;
        newRoot->children[1] = rightHalf;
        root = newRoot;
    }
    keys++;
}

// Inserts below node. Returns true if node split; then upKey and rightHalf
// are what the parent must add (upKey goes between node and rightHalf).
bool BPlusTree::insertInto(BNode* node, const string& token, DynArray<SearchPosting>* list,
                           string& upKey, BNode*& rightHalf) {
    if (node->leaf) {
        //shift bigger keys right and put the token in its sorted place
        int i = node->count;
        while (i > 0 && token < node->keys[i - 1]) {
            node->keys[i] = node->keys[i - 1];
            node->lists[i] = node->lists[i - 1];
            i--;
        }
        node->keys[i] = token;
        node->lists[i] = list;
        node->count++;
        if (node->count > BPLUS_MAX_KEYS) {
            splitLeaf(node, upKey, rightHalf);
            return true;
        }
        return false;
    }

    int c = childIndex(node, token);
    string childKey;
    BNode* childRight = nullptr;
    if (!insertInto(node->children[c], token, list, childKey, childRight)) {
        return false;
    }
    //the child split: add its separator at c and its new right half at c + 1
    for (int i = node->count; i > c; i--) {
        node->keys[i] = node->keys[i - 1];
        node->children[i + 1] = node->children[i];
    }
    node->keys[c] = childKey;
    node->children[c + 1] = childRight;
    node->count++;
    if (node->count > BPLUS_MAX_KEYS) {
        splitInternal(node, upKey, rightHalf);
        return true;
    }
    return false;
}

// An overfull leaf keeps its first half and moves the rest to a new leaf.
// The new leaf's first key is COPIED up: it stays in the leaf too, because
// leaves hold every key.
void BPlusTree::splitLeaf(BNode* leaf, string& upKey, BNode*& rightHalf) {
    int keep = leaf->count - leaf->count / 2;
    BNode* right = newNode(true);
    for (int i = keep; i < leaf->count; i++) {
        right->keys[right->count] = leaf->keys[i];
        right->lists[right->count] = leaf->lists[i];
        right->count++;
        leaf->lists[i] = nullptr;
    }
    leaf->count = keep;

    //link the new leaf into the chain
    right->next = leaf->next;
    leaf->next = right;

    upKey = right->keys[0];
    rightHalf = right;
}

// An overfull internal node splits around its middle key, which MOVES up:
// it only guides the descent, so it is not needed below any more.
void BPlusTree::splitInternal(BNode* node, string& upKey, BNode*& rightHalf) {
    int mid = node->count / 2;
    BNode* right = newNode(false);
    for (int i = mid + 1; i < node->count; i++) {
        right->keys[right->count] = node->keys[i];
        right->children[right->count] = node->children[i];
        right->count++;
    }
    right->children[right->count] = node->children[node->count];
    upKey = node->keys[mid];
    node->count = mid;
    rightHalf = right;
}

// ---------- remove ----------

void BPlusTree::removePosting(const string& token, int file, int line) {
    DynArray<SearchPosting>* list = lookup(token);
    if (list == nullptr) {
        return;
    }
    int pos = lowerBound(*list, file, line);
    if (pos == list->size() || !samePosting(list->get(pos), file, line)) {
        return;  // the pair is not there
    }
    for (int i = pos; i < list->size() - 1; i++) {
        list->set(i, list->get(i + 1));
    }
    list->remove();

    if (!list->isEmpty()) {
        return;
    }
    //the list is empty: the key goes
    removeFrom(root, token);
    keys--;

    //the root lost its last separator: its only child becomes the root,
    //the only way the tree gets shorter
    if (!root->leaf && root->count == 0) {
        BNode* old = root;
        root = root->children[0];
        delete old;
    }
}

// Removes token from the subtree under node. A child left too empty is fixed
// on the way back up, so only the root can ever be left under the minimum.
void BPlusTree::removeFrom(BNode* node, const string& token) {
    if (node->leaf) {
        int i = 0;
        while (i < node->count && node->keys[i] != token) {
            i++;
        }
        if (i == node->count) {
            return;
        }
        delete node->lists[i];
        for (int j = i; j < node->count - 1; j++) {
            node->keys[j] = node->keys[j + 1];
            node->lists[j] = node->lists[j + 1];
        }
        node->lists[node->count - 1] = nullptr;
        node->count--;
        return;
    }

    int c = childIndex(node, token);
    removeFrom(node->children[c], token);
    if (underflow(node->children[c])) {
        fixChild(node, c);
    }
}

bool BPlusTree::underflow(const BNode* node) const {
    int min = node->leaf ? BPLUS_MIN_LEAF_KEYS : BPLUS_MIN_INTERNAL_KEYS;
    return node->count < min;
}

// a sibling can lend a key if it stays at or above the minimum afterwards
bool BPlusTree::canLend(const BNode* node) const {
    int min = node->leaf ? BPLUS_MIN_LEAF_KEYS : BPLUS_MIN_INTERNAL_KEYS;
    return node->count > min;
}

// Child i of parent is one key short. Borrow from a sibling if one has a
// key to spare; otherwise merge with a sibling.
void BPlusTree::fixChild(BNode* parent, int i) {
    if (i > 0 && canLend(parent->children[i - 1])) {
        borrowFromLeft(parent, i);
    }
    else if (i < parent->count && canLend(parent->children[i + 1])) {
        borrowFromRight(parent, i);
    }
    else if (i > 0) {
        mergeChildren(parent, i - 1);
    }
    else {
        mergeChildren(parent, i);
    }
}

void BPlusTree::borrowFromLeft(BNode* parent, int i) {
    BNode* child = parent->children[i];
    BNode* left = parent->children[i - 1];

    if (child->leaf) {
        //move the left sibling's last key to the front of child
        for (int j = child->count; j > 0; j--) {
            child->keys[j] = child->keys[j - 1];
            child->lists[j] = child->lists[j - 1];
        }
        child->keys[0] = left->keys[left->count - 1];
        child->lists[0] = left->lists[left->count - 1];
        left->lists[left->count - 1] = nullptr;
        child->count++;
        left->count--;
        //child now starts with a smaller key, so the separator follows it
        parent->keys[i - 1] = child->keys[0];
        return;
    }

    //internal: rotate through the parent. The separator comes down into
    //child, and the left sibling's last key goes up to replace it.
    for (int j = child->count; j > 0; j--) {
        child->keys[j] = child->keys[j - 1];
    }
    for (int j = child->count + 1; j > 0; j--) {
        child->children[j] = child->children[j - 1];
    }
    child->keys[0] = parent->keys[i - 1];
    child->children[0] = left->children[left->count];
    child->count++;
    parent->keys[i - 1] = left->keys[left->count - 1];
    left->count--;
}

void BPlusTree::borrowFromRight(BNode* parent, int i) {
    BNode* child = parent->children[i];
    BNode* right = parent->children[i + 1];

    if (child->leaf) {
        //move the right sibling's first key to the end of child
        child->keys[child->count] = right->keys[0];
        child->lists[child->count] = right->lists[0];
        child->count++;
        for (int j = 0; j < right->count - 1; j++) {
            right->keys[j] = right->keys[j + 1];
            right->lists[j] = right->lists[j + 1];
        }
        right->lists[right->count - 1] = nullptr;
        right->count--;
        //the right sibling now starts later, so the separator follows it
        parent->keys[i] = right->keys[0];
        return;
    }

    //internal: rotate through the parent the other way
    child->keys[child->count] = parent->keys[i];
    child->children[child->count + 1] = right->children[0];
    child->count++;
    parent->keys[i] = right->keys[0];
    for (int j = 0; j < right->count - 1; j++) {
        right->keys[j] = right->keys[j + 1];
    }
    for (int j = 0; j < right->count; j++) {
        right->children[j] = right->children[j + 1];
    }
    right->count--;
}

// Merges child i + 1 of parent into child i and drops the separator between
// them from the parent. The parent may now be short; its own parent fixes that.
void BPlusTree::mergeChildren(BNode* parent, int i) {
    BNode* left = parent->children[i];
    BNode* right = parent->children[i + 1];

    if (left->leaf) {
        //leaves: append the keys and take right out of the leaf chain
        for (int j = 0; j < right->count; j++) {
            left->keys[left->count + j] = right->keys[j];
            left->lists[left->count + j] = right->lists[j];
        }
        left->count += right->count;
        left->next = right->next;
    }
    else {
        //internal: the separator comes down between the two halves
        left->keys[left->count] = parent->keys[i];
        for (int j = 0; j < right->count; j++) {
            left->keys[left->count + 1 + j] = right->keys[j];
        }
        for (int j = 0; j <= right->count; j++) {
            left->children[left->count + 1 + j] = right->children[j];
        }
        left->count += 1 + right->count;
    }
    delete right;  // its keys, lists and children now belong to left

    //remove separator i and child i + 1 from the parent
    for (int j = i; j < parent->count - 1; j++) {
        parent->keys[j] = parent->keys[j + 1];
    }
    for (int j = i + 1; j < parent->count; j++) {
        parent->children[j] = parent->children[j + 1];
    }
    parent->count--;
}

// ---------- for tests and the demo ----------

int BPlusTree::keyCount() const {
    return keys;
}

int BPlusTree::height() const {
    int h = 1;
    const BNode* node = root;
    while (!node->leaf) {
        node = node->children[0];
        h++;
    }
    return h;
}

// Checks one subtree. Every key in it must be >= *low and < *high (a null
// bound means no limit). leafDepth is the depth of the first leaf seen, and
// every other leaf must match it.
bool BPlusTree::checkNode(const BNode* node, int depth, int& leafDepth, const string* low,
                          const string* high, string& why) const {
    if (node->count > BPLUS_MAX_KEYS) {
        why = "a node has too many keys";
        return false;
    }
    if (node != root && underflow(node)) {
        why = "a node other than the root has too few keys";
        return false;
    }
    for (int i = 0; i < node->count; i++) {
        if (i > 0 && !(node->keys[i - 1] < node->keys[i])) {
            why = "keys in a node are not in increasing order";
            return false;
        }
        if ((low != nullptr && node->keys[i] < *low) || (high != nullptr && !(node->keys[i] < *high))) {
            why = "key \"" + node->keys[i] + "\" is on the wrong side of a separator";
            return false;
        }
    }

    if (node->leaf) {
        if (leafDepth == -1) {
            leafDepth = depth;
        }
        if (depth != leafDepth) {
            why = "leaves are not all at the same depth";
            return false;
        }
        for (int i = 0; i < node->count; i++) {
            const DynArray<SearchPosting>* list = node->lists[i];
            if (list == nullptr || list->isEmpty()) {
                why = "key \"" + node->keys[i] + "\" has no postings";
                return false;
            }
            for (int j = 1; j < list->size(); j++) {
                const SearchPosting& p = list->get(j);
                if (!before(list->get(j - 1), p.file, p.line)) {
                    why = "postings of \"" + node->keys[i] + "\" are not sorted or repeat";
                    return false;
                }
            }
        }
        return true;
    }

    if (node == root && node->count == 0) {
        why = "the root is an internal node with no keys";
        return false;
    }
    for (int i = 0; i <= node->count; i++) {
        const string* childLow = (i == 0) ? low : &node->keys[i - 1];
        const string* childHigh = (i == node->count) ? high : &node->keys[i];
        if (!checkNode(node->children[i], depth + 1, leafDepth, childLow, childHigh, why)) {
            return false;
        }
    }
    return true;
}

bool BPlusTree::validate(string& why) const {
    int leafDepth = -1;
    if (!checkNode(root, 0, leafDepth, nullptr, nullptr, why)) {
        return false;
    }

    //the leaf chain must visit every key exactly once, in increasing order
    const BNode* leaf = root;
    while (!leaf->leaf) {
        leaf = leaf->children[0];
    }
    int seen = 0;
    const string* last = nullptr;
    while (leaf != nullptr) {
        for (int i = 0; i < leaf->count; i++) {
            if (last != nullptr && !(*last < leaf->keys[i])) {
                why = "the leaf chain is not in increasing order";
                return false;
            }
            last = &leaf->keys[i];
            seen++;
        }
        leaf = leaf->next;
    }
    if (seen != keys) {
        why = "the leaf chain has " + to_string(seen) + " keys but the tree counts " + to_string(keys);
        return false;
    }
    return true;
}

void BPlusTree::describeLevel(const BNode* node, int level, int target, string& out) const {
    if (level == target) {
        if (!out.empty()) {
            out += " ";
        }
        out += "[";
        for (int i = 0; i < node->count; i++) {
            if (i > 0) {
                out += " ";
            }
            out += node->keys[i];
        }
        out += "]";
        return;
    }
    for (int i = 0; i <= node->count; i++) {
        describeLevel(node->children[i], level + 1, target, out);
    }
}

string BPlusTree::describe() const {
    string result;
    int h = height();
    for (int level = 0; level < h; level++) {
        string line;
        describeLevel(root, 0, level, line);
        result += line + "\n";
    }
    return result;
}
