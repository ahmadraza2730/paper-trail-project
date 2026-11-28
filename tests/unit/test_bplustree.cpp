// Unit tests for the B+ tree word index (spec Section 5.7.1).
// Build and run from the project root with `make unit`.
//
// Every test checks the B+ tree rules with validate() after each change:
// node sizes within limits, all leaves at the same depth, keys on the right
// side of every separator, and a leaf chain that visits every key in order.
// The random test also compares every answer against a plain array model.

#include "search/BPlusTree.h"
#include "search/Search.h"
#include "foundation/AllocTracker.h"
#include "foundation/InsertionSort.h"
#include <iostream>

using namespace std;

static int passed = 0;
static int failed = 0;

static void check(bool ok, const string& what) {
    if (ok) {
        passed++;
    } else {
        failed++;
        cout << "FAIL: " << what << endl;
    }
}

static void checkText(const string& got, const string& expected, const string& what) {
    if (got == expected) {
        passed++;
        return;
    }
    failed++;
    cout << "FAIL: " << what << endl;
    cout << "--- expected ---" << endl << expected << "--- got ---" << endl << got;
}

static void checkValid(const BPlusTree& tree, const string& when) {
    string why;
    check(tree.validate(why), when + ": " + why);
}

// "(1,2) (1,5)" or "none"
static string postingsText(const BPlusTree& tree, const string& token) {
    const DynArray<SearchPosting>* list = tree.find(token);
    if (list == nullptr) {
        return "none";
    }
    string text;
    for (int i = 0; i < list->size(); i++) {
        if (i > 0) {
            text += " ";
        }
        text += "(" + to_string(list->get(i).file) + "," + to_string(list->get(i).line) + ")";
    }
    return text;
}

// collects what scanPrefix reports: one posting list per key, in order
struct ScanResult {
    string text;
    int calls;
};

static void collect(const DynArray<SearchPosting>& postings, void* context) {
    ScanResult* result = (ScanResult*)context;
    result->calls++;
    if (!result->text.empty()) {
        result->text += " | ";
    }
    for (int i = 0; i < postings.size(); i++) {
        if (i > 0) {
            result->text += " ";
        }
        result->text += "(" + to_string(postings.get(i).file) + "," + to_string(postings.get(i).line) + ")";
    }
}

static string scanText(const BPlusTree& tree, const string& prefix) {
    ScanResult result;
    result.calls = 0;
    tree.scanPrefix(prefix, collect, &result);
    return result.text;
}

// ---------- small cases ----------

static void testEmptyTree() {
    BPlusTree tree;
    check(tree.find("honey") == nullptr, "empty tree finds nothing");
    check(tree.keyCount() == 0 && tree.height() == 1, "empty tree is one empty leaf");
    checkText(scanText(tree, "h"), "", "prefix scan of an empty tree");
    tree.removePosting("honey", 1, 1);
    checkValid(tree, "remove from an empty tree");
}

static void testPostingListsStaySorted() {
    BPlusTree tree;
    tree.addPosting("honey", 2, 7);
    tree.addPosting("honey", 1, 9);
    tree.addPosting("honey", 2, 3);
    tree.addPosting("honey", 1, 2);
    tree.addPosting("honey", 2, 3);  // repeat: no change
    checkText(postingsText(tree, "honey"), "(1,2) (1,9) (2,3) (2,7)", "postings sorted by file then line, no repeats");
    check(tree.keyCount() == 1, "one key for one token");

    tree.removePosting("honey", 1, 9);
    tree.removePosting("honey", 5, 5);  // not there: no change
    checkText(postingsText(tree, "honey"), "(1,2) (2,3) (2,7)", "remove one posting");

    tree.removePosting("honey", 1, 2);
    tree.removePosting("honey", 2, 3);
    tree.removePosting("honey", 2, 7);
    check(tree.find("honey") == nullptr && tree.keyCount() == 0, "key goes when its list empties");
    checkValid(tree, "after emptying a list");
}

// the exact shape after each split, with order 4 (at most 3 keys a node)
static void testSplitsGrowTheTree() {
    BPlusTree tree;
    tree.addPosting("apple", 1, 1);
    tree.addPosting("bread", 1, 2);
    tree.addPosting("honey", 1, 3);
    checkText(tree.describe(), "[apple bread honey]\n", "three keys fit in one leaf");

    //a fourth key overflows the leaf: it splits and "honey" is copied up
    tree.addPosting("milk", 1, 4);
    checkText(tree.describe(), "[honey]\n[apple bread] [honey milk]\n", "leaf split makes a new root");
    check(tree.height() == 2, "height 2 after the first split");

    tree.addPosting("oats", 1, 5);
    tree.addPosting("pear", 1, 6);
    tree.addPosting("rice", 1, 7);
    tree.addPosting("salt", 1, 8);
    tree.addPosting("tea", 1, 9);
    checkText(tree.describe(), "[honey oats rice]\n[apple bread] [honey milk] [oats pear] [rice salt tea]\n",
              "more leaf splits fill the root");

    //the next leaf split overflows the root ([honey oats rice tea]):
    //it splits and its middle key "rice" moves up into a new root
    tree.addPosting("yam", 1, 10);
    checkText(tree.describe(),
              "[rice]\n[honey oats] [tea]\n[apple bread] [honey milk] [oats pear] [rice salt] [tea yam]\n",
              "internal split moves the middle key up");
    check(tree.height() == 3, "height 3 after the root splits");
    checkValid(tree, "after splits");
}

// removing keys borrows from siblings, merges, and finally shrinks the tree
static void testMergesShrinkTheTree() {
    BPlusTree tree;
    const char* words[] = {"apple", "bread", "honey", "milk", "oats", "pear", "rice", "salt", "tea", "yam"};
    for (int i = 0; i < 10; i++) {
        tree.addPosting(words[i], 1, i + 1);
    }

    //[rice] is short after "salt" goes and [tea yam] has none to spare, so they
    //merge. That leaves its parent with no keys, so the parent borrows "oats"
    //from its left sibling through the root (a rotation).
    tree.removePosting("salt", 1, 8);
    checkText(tree.describe(), "[oats]\n[honey] [rice]\n[apple bread] [honey milk] [oats pear] [rice tea yam]\n",
              "leaf merge");
    checkValid(tree, "after a leaf merge");

    //[oats pear] is short after "pear" goes; its right sibling has a key to lend
    tree.removePosting("pear", 1, 6);
    checkText(tree.describe(), "[oats]\n[honey] [tea]\n[apple bread] [honey milk] [oats rice] [tea yam]\n",
              "borrow from the right sibling updates the separator");
    checkValid(tree, "after a borrow");

    for (int i = 0; i < 10; i++) {
        tree.removePosting(words[i], 1, i + 1);
        checkValid(tree, string("after removing ") + words[i]);
    }
    check(tree.keyCount() == 0 && tree.height() == 1, "removing every key shrinks back to one leaf");
}

static void testPrefixScan() {
    BPlusTree tree;
    //enough keys that "ho" keys span several leaves
    const char* words[] = {"hob", "hod", "hog", "hold", "hole", "home", "hone", "honey", "hop", "hot",
                           "hat", "him", "house", "hum", "ant", "zoo"};
    for (int i = 0; i < 16; i++) {
        tree.addPosting(words[i], 1, i + 1);
    }
    checkValid(tree, "prefix tree");
    check(tree.height() >= 3, "prefix keys span several levels");

    checkText(scanText(tree, "hon"), "(1,7) | (1,8)", "prefix hon: hone, honey");
    checkText(scanText(tree, "ho"), "(1,1) | (1,2) | (1,3) | (1,4) | (1,5) | (1,6) | (1,7) | (1,8) | (1,9) | (1,10) | (1,13)",
              "prefix ho walks along the leaves in key order");
    checkText(scanText(tree, "honey"), "(1,8)", "a whole word is its own prefix");
    checkText(scanText(tree, "hx"), "", "no keys with prefix");
    checkText(scanText(tree, "zzz"), "", "prefix past every key");
    checkText(scanText(tree, "a"), "(1,15)", "prefix before every other key");
}

// the index helpers in Search.cpp, now on the real tree
static void testWithSearchHelpers() {
    BPlusTree tree;
    indexInsertLine(tree, 1, 1, "Honey and oats, honey!");
    indexInsertLine(tree, 2, 1, "oats only");
    checkText(postingsText(tree, "honey"), "(1,1)", "repeated word in one line is one posting");
    checkText(postingsText(tree, "oats"), "(1,1) (2,1)", "word in two documents");

    indexEditLine(tree, 1, 1, "Honey and oats, honey!", "milk and oats");
    checkText(postingsText(tree, "honey"), "none", "edit drops a word");
    checkText(postingsText(tree, "milk"), "(1,1)", "edit adds a word");

    indexDeleteLine(tree, 2, 1, "oats only");
    checkText(postingsText(tree, "oats"), "(1,1)", "delete drops the line");
    checkText(postingsText(tree, "only"), "none", "delete drops a word only that line had");
    checkValid(tree, "after the search helpers");
}

// ---------- random operations against a simple model ----------

// xorshift: a small, fast pseudo-random generator, so runs are repeatable
static unsigned int seed = 12345;

static int randomInt(int n) {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return (int)(seed % (unsigned int)n);
}

const int WORDS = 120;
const int FILES = 2;
const int LINES = 2;

static bool comesBefore(const string& a, const string& b) {
    return a < b;
}

static void testRandomAgainstModel(unsigned int startSeed) {
    seed = startSeed;
    //random 1 to 4 letter words over a small alphabet, so many share prefixes
    DynArray<string> vocabulary;
    while (vocabulary.size() < WORDS) {
        string word;
        int length = 1 + randomInt(4);
        for (int i = 0; i < length; i++) {
            word += (char)('a' + randomInt(4));
        }
        bool repeat = false;
        for (int i = 0; i < vocabulary.size(); i++) {
            repeat = repeat || vocabulary.get(i) == word;
        }
        if (!repeat) {
            vocabulary.add(word);
        }
    }
    insertionSort(vocabulary, comesBefore);

    //the model: has[w][f][l] is true when (f, l) is in word w's list.
    //Few files and lines, so lists often empty and keys really get removed.
    static bool has[WORDS][FILES][LINES];
    for (int w = 0; w < WORDS; w++) {
        for (int f = 0; f < FILES; f++) {
            for (int l = 0; l < LINES; l++) {
                has[w][f][l] = false;
            }
        }
    }

    BPlusTree tree;
    int maxHeight = 0;
    for (int step = 0; step < 20000; step++) {
        int w = randomInt(WORDS);
        int f = randomInt(FILES);
        int l = randomInt(LINES);
        //add more than remove at first so the tree grows, then the other way round
        bool add = randomInt(100) < (step < 10000 ? 65 : 35);
        if (add) {
            tree.addPosting(vocabulary.get(w), f, l);
        }
        else {
            tree.removePosting(vocabulary.get(w), f, l);
        }
        has[w][f][l] = add;

        string why;
        if (!tree.validate(why)) {
            check(false, "seed " + to_string(startSeed) + " step " + to_string(step) + ": " + why);
            return;
        }
        if (tree.height() > maxHeight) {
            maxHeight = tree.height();
        }

        if (step % 500 != 0) {
            continue;
        }
        //every 500 steps, compare every list, the key count and some prefix scans
        int expectedKeys = 0;
        bool same = true;
        for (int v = 0; v < WORDS; v++) {
            string expected;
            for (int a = 0; a < FILES; a++) {
                for (int b = 0; b < LINES; b++) {
                    if (has[v][a][b]) {
                        if (!expected.empty()) {
                            expected += " ";
                        }
                        expected += "(" + to_string(a) + "," + to_string(b) + ")";
                    }
                }
            }
            if (expected.empty()) {
                expected = "none";
            }
            else {
                expectedKeys++;
            }
            same = same && postingsText(tree, vocabulary.get(v)) == expected;
        }
        check(same, "seed " + to_string(startSeed) + " step " + to_string(step) + ": every posting list matches the model");
        check(tree.keyCount() == expectedKeys, "seed " + to_string(startSeed) + " step " + to_string(step) + ": key count matches the model");

        const char* prefixes[] = {"a", "b", "ab", "cd", "dda"};
        for (int p = 0; p < 5; p++) {
            string prefix = prefixes[p];
            string expected;
            for (int v = 0; v < WORDS; v++) {
                if (vocabulary.get(v).compare(0, prefix.length(), prefix) != 0) {
                    continue;
                }
                string text = postingsText(tree, vocabulary.get(v));
                if (text == "none") {
                    continue;
                }
                if (!expected.empty()) {
                    expected += " | ";
                }
                expected += text;
            }
            checkText(scanText(tree, prefix), expected, "seed " + to_string(startSeed) + " step " + to_string(step) + ": prefix " + prefix);
        }
    }
    check(maxHeight >= 4, "random test grew the tree to at least 4 levels");
}

int main() {
    long long liveBefore = liveAllocations();

    testEmptyTree();
    testPostingListsStaySorted();
    testSplitsGrowTheTree();
    testMergesShrinkTheTree();
    testPrefixScan();
    testWithSearchHelpers();
    testRandomAgainstModel(12345);
    testRandomAgainstModel(777);
    testRandomAgainstModel(2026);

    long long leaked = liveAllocations() - liveBefore;
    check(leaked == 0, "no leaked allocations");

    cout << "bplustree: " << passed << " passed, " << failed << " failed" << endl;
    return failed == 0 ? 0 : 1;
}
