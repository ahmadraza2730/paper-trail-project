// Unit tests for search (spec Section 5.7).
// Build and run from the project root with `make unit`, or by hand:
//   g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Isrc tests/unit/test_search.cpp
//       src/search/Search.cpp src/filters/BloomFilter.cpp src/foundation/Hash.cpp
//       src/foundation/Tokenizer.cpp src/foundation/AllocTracker.cpp -o test_search
//   ./test_search
//
// The B+ tree and the documents are not in main yet, so these tests use two
// small stand-ins: FakeIndex (a sorted array of keys, which answers like the
// B+ tree's leaf level) and FakeDocs (each document's working lines and the
// Bloom filter of its head commit).

#include "search/Search.h"
#include "foundation/AllocTracker.h"
#include "foundation/Tokenizer.h"
#include <iostream>
#include <sstream>

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

// ---------- stand-in for the B+ tree ----------

class FakeIndex : public WordIndex {
private:
    DynArray<string> keys;                    // sorted
    DynArray<DynArray<SearchPosting>*> lists; // lists.get(i) belongs to keys.get(i)

    // position of the first key >= token
    int lowerBound(const string& token) const {
        int i = 0;
        while (i < keys.size() && keys.get(i) < token) {
            i++;
        }
        return i;
    }

public:
    mutable int lookups = 0;

    FakeIndex() {}
    ~FakeIndex() {
        for (int i = 0; i < lists.size(); i++) {
            delete lists.get(i);
        }
    }
    FakeIndex(const FakeIndex&) = delete;
    FakeIndex& operator=(const FakeIndex&) = delete;

    void addPosting(const string& token, int file, int line) override {
        int k = lowerBound(token);
        if (k == keys.size() || keys.get(k) != token) {
            //new key: shift the later keys right by one
            keys.add(token);
            lists.add(nullptr);
            for (int i = keys.size() - 1; i > k; i--) {
                keys.set(i, keys.get(i - 1));
                lists.set(i, lists.get(i - 1));
            }
            keys.set(k, token);
            lists.set(k, new DynArray<SearchPosting>());
        }
        DynArray<SearchPosting>& list = *lists.get(k);
        int p = 0;
        while (p < list.size() && (list.get(p).file < file ||
                                   (list.get(p).file == file && list.get(p).line < line))) {
            p++;
        }
        if (p < list.size() && list.get(p).file == file && list.get(p).line == line) {
            return;
        }
        SearchPosting added = {file, line};
        list.add(added);
        for (int i = list.size() - 1; i > p; i--) {
            list.set(i, list.get(i - 1));
        }
        list.set(p, added);
    }

    void removePosting(const string& token, int file, int line) override {
        int k = lowerBound(token);
        if (k == keys.size() || keys.get(k) != token) {
            return;
        }
        DynArray<SearchPosting>& list = *lists.get(k);
        for (int p = 0; p < list.size(); p++) {
            if (list.get(p).file == file && list.get(p).line == line) {
                for (int i = p; i + 1 < list.size(); i++) {
                    list.set(i, list.get(i + 1));
                }
                list.remove();
                break;
            }
        }
        if (list.isEmpty()) {
            delete lists.get(k);
            for (int i = k; i + 1 < keys.size(); i++) {
                keys.set(i, keys.get(i + 1));
                lists.set(i, lists.get(i + 1));
            }
            keys.remove();
            lists.remove();
        }
    }

    const DynArray<SearchPosting>* find(const string& token) const override {
        lookups++;
        int k = lowerBound(token);
        if (k == keys.size() || keys.get(k) != token) {
            return nullptr;
        }
        return lists.get(k);
    }

    void scanPrefix(const string& prefix, PostingListFn fn, void* context) const override {
        for (int k = lowerBound(prefix); k < keys.size(); k++) {
            if (keys.get(k).compare(0, prefix.length(), prefix) != 0) {
                break;
            }
            fn(*lists.get(k), context);
        }
    }

    bool hasKey(const string& token) const {
        int k = lowerBound(token);
        return k < keys.size() && keys.get(k) == token;
    }

    // "(1,2) (2,2)" for a key, or "-" if it is absent
    string postingsText(const string& token) const {
        int k = lowerBound(token);
        if (k == keys.size() || keys.get(k) != token) {
            return "-";
        }
        string out = "";
        const DynArray<SearchPosting>& list = *lists.get(k);
        for (int i = 0; i < list.size(); i++) {
            if (i > 0) {
                out += " ";
            }
            out += "(" + to_string(list.get(i).file) + "," + to_string(list.get(i).line) + ")";
        }
        return out;
    }
};

// ---------- stand-in for documents and their head commits ----------

struct FakeDoc {
    int id;
    string name;
    bool live;
    DynArray<int> lineIds;    // working text, in line order
    DynArray<string> authors;
    DynArray<string> texts;
    BloomFilter* head;        // owned by FakeDocs
};

class FakeDocs : public SearchDocuments {
private:
    DynArray<FakeDoc*> docs;  // FileID order
    DynArray<BloomFilter*> filters;

    const FakeDoc* byId(int file) const {
        for (int i = 0; i < docs.size(); i++) {
            if (docs.get(i)->id == file) {
                return docs.get(i);
            }
        }
        return nullptr;
    }

public:
    FakeDocs() {}
    ~FakeDocs() {
        for (int i = 0; i < docs.size(); i++) {
            delete docs.get(i);
        }
        for (int i = 0; i < filters.size(); i++) {
            delete filters.get(i);
        }
    }
    FakeDocs(const FakeDocs&) = delete;
    FakeDocs& operator=(const FakeDocs&) = delete;

    FakeDoc& add(int id, const string& name) {
        FakeDoc* d = new FakeDoc();
        d->id = id;
        d->name = name;
        d->live = true;
        d->head = nullptr;
        docs.add(d);
        return *d;
    }

    FakeDoc& doc(int id) {
        return *(FakeDoc*)byId(id);
    }

    // a filter owned here (commits share filters, as a fork shares its head)
    BloomFilter* newFilter(const BloomFilter* parent) {
        BloomFilter* f = parent == nullptr ? new BloomFilter(32768, 3) : new BloomFilter(*parent);
        filters.add(f);
        return f;
    }

    int liveCount() const override {
        int n = 0;
        for (int i = 0; i < docs.size(); i++) {
            if (docs.get(i)->live) {
                n++;
            }
        }
        return n;
    }

    int liveFileId(int i) const override {
        for (int d = 0; d < docs.size(); d++) {
            if (docs.get(d)->live) {
                if (i == 0) {
                    return docs.get(d)->id;
                }
                i--;
            }
        }
        return -1;
    }

    const BloomFilter* headFilter(int file) const override {
        return byId(file)->head;
    }

    string name(int file) const override {
        return byId(file)->name;
    }

    bool workingLine(int file, int line, int& number, string& author, string& text) const override {
        const FakeDoc* d = byId(file);
        for (int i = 0; i < d->lineIds.size(); i++) {
            if (d->lineIds.get(i) == line) {
                number = i + 1;
                author = d->authors.get(i);
                text = d->texts.get(i);
                return true;
            }
        }
        return false;
    }
};

// ---------- helpers that play the part of the document code ----------

static void addWorkingLine(FakeDoc& d, int at, int lineId, const string& author, const string& text) {
    // at is 0-based: the new line goes before the current line at
    d.lineIds.add(lineId);
    d.authors.add(author);
    d.texts.add(text);
    for (int i = d.lineIds.size() - 1; i > at; i--) {
        d.lineIds.set(i, d.lineIds.get(i - 1));
        d.authors.set(i, d.authors.get(i - 1));
        d.texts.set(i, d.texts.get(i - 1));
    }
    d.lineIds.set(at, lineId);
    d.authors.set(at, author);
    d.texts.set(at, text);
}

static void removeWorkingLine(FakeDoc& d, int lineId) {
    for (int i = 0; i < d.lineIds.size(); i++) {
        if (d.lineIds.get(i) == lineId) {
            for (int j = i; j + 1 < d.lineIds.size(); j++) {
                d.lineIds.set(j, d.lineIds.get(j + 1));
                d.authors.set(j, d.authors.get(j + 1));
                d.texts.set(j, d.texts.get(j + 1));
            }
            d.lineIds.remove();
            d.authors.remove();
            d.texts.remove();
            return;
        }
    }
}

static void editWorkingLine(FakeDoc& d, int lineId, const string& author, const string& text) {
    for (int i = 0; i < d.lineIds.size(); i++) {
        if (d.lineIds.get(i) == lineId) {
            d.authors.set(i, author);
            d.texts.set(i, text);
        }
    }
}

// what a save does to a commit's filter: add the tokens of new text
static void addTokens(BloomFilter* f, const string& text) {
    DynArray<string> tokens = tokenize(text);
    for (int i = 0; i < tokens.size(); i++) {
        f->add(tokens.get(i));
    }
}

static const char* PANCAKES = "Pancakes need eggs and flour.";
static const char* HONEY = "Serve with honey.";
static const char* COOK = "Cook five minutes per side.";
static const char* MAPLE = "Serve with maple syrup.";
static const char* BERRIES = "Top with berries.";
static const char* WARM = "Best served warm.";

// Builds the state after Section 5.5 (before any merge):
// recipes.txt (F1) at c4, vegan.txt (F2) at c3, notes.txt (F3).
// Every step does what save and fork will do to the index and the filters.
static void buildSpecState(FakeIndex& index, FakeDocs& docs) {
    FakeDoc& recipes = docs.add(1, "recipes.txt");
    FakeDoc& vegan = docs.add(2, "vegan.txt");
    FakeDoc& notes = docs.add(3, "notes.txt");

    // recipes.txt: three adds, save -> c2
    addWorkingLine(recipes, 0, 1, "alice", PANCAKES);
    addWorkingLine(recipes, 1, 2, "bob", HONEY);
    addWorkingLine(recipes, 2, 3, "alice", COOK);
    BloomFilter* c2 = docs.newFilter(nullptr);
    indexInsertLine(index, 1, 1, PANCAKES);
    indexInsertLine(index, 1, 2, HONEY);
    indexInsertLine(index, 1, 3, COOK);
    addTokens(c2, PANCAKES);
    addTokens(c2, HONEY);
    addTokens(c2, COOK);
    recipes.head = c2;

    // fork recipes.txt vegan.txt: same LineIDs, copied postings, shared head c2
    addWorkingLine(vegan, 0, 1, "alice", PANCAKES);
    addWorkingLine(vegan, 1, 2, "bob", HONEY);
    addWorkingLine(vegan, 2, 3, "alice", COOK);
    indexInsertLine(index, 2, 1, PANCAKES);
    indexInsertLine(index, 2, 2, HONEY);
    indexInsertLine(index, 2, 3, COOK);
    vegan.head = c2;

    // vegan.txt: carol edits line 2 and adds L4, save -> c3
    editWorkingLine(vegan, 2, "carol", MAPLE);
    addWorkingLine(vegan, 3, 4, "carol", BERRIES);
    BloomFilter* c3 = docs.newFilter(c2);
    indexEditLine(index, 2, 2, HONEY, MAPLE);
    indexInsertLine(index, 2, 4, BERRIES);
    addTokens(c3, MAPLE);
    addTokens(c3, BERRIES);
    vegan.head = c3;

    // recipes.txt: alice adds L5, save -> c4
    addWorkingLine(recipes, 3, 5, "alice", WARM);
    BloomFilter* c4 = docs.newFilter(c2);
    indexInsertLine(index, 1, 5, WARM);
    addTokens(c4, WARM);
    recipes.head = c4;

    // notes.txt: one saved line
    addWorkingLine(notes, 0, 6, "alice", "Hi.");
    BloomFilter* n2 = docs.newFilter(nullptr);
    indexInsertLine(index, 3, 6, "Hi.");
    addTokens(n2, "Hi.");
    notes.head = n2;
}

// runs search and returns what it printed
static string captureSearch(const string& term, const WordIndex& index, const SearchDocuments& docs) {
    ostringstream out;
    streambuf* old = cout.rdbuf(out.rdbuf());
    runSearch(term, index, docs);
    cout.rdbuf(old);
    return out.str();
}

// ---------- tests ----------

// Section 5.7.2 transcript
static void testSpecTranscript() {
    FakeIndex index;
    FakeDocs docs;
    buildSpecState(index, docs);

    checkText(captureSearch("honey", index, docs),
              "recipes.txt:2 [bob] Serve with honey.\n"
              "1 match in 1 document (bloom: 2 of 3 documents passed)\n",
              "search honey (spec 5.7.2)");

    checkText(captureSearch("pan*", index, docs),
              "recipes.txt:1 [alice] Pancakes need eggs and flour.\n"
              "vegan.txt:1 [alice] Pancakes need eggs and flour.\n"
              "2 matches in 2 documents (bloom: skipped for prefix queries)\n",
              "search pan* (spec 5.7.2)");
}

// vegan.txt passes the honey filter only because c3 copied c2's bits
static void testFalsePositiveExplained() {
    FakeIndex index;
    FakeDocs docs;
    buildSpecState(index, docs);
    check(docs.headFilter(2)->mightContain("honey"), "vegan.txt filter passes honey (copied from c2)");
    check(!docs.headFilter(3)->mightContain("honey"), "notes.txt filter rejects honey");
    check(index.postingsText("honey") == "(1,2)", "B+ tree has no honey posting for vegan.txt");
}

// Section 5.7.1 EditLine example
static void testEditUpdatesIndex() {
    FakeIndex index;
    indexInsertLine(index, 1, 2, HONEY);
    indexInsertLine(index, 2, 2, HONEY);
    check(index.postingsText("honey") == "(1,2) (2,2)", "before c3: honey -> (F1,L2) (F2,L2)");

    indexEditLine(index, 2, 2, HONEY, MAPLE);
    check(index.postingsText("honey") == "(1,2)", "after: honey -> (F1,L2)");
    check(index.postingsText("maple") == "(2,2)", "after: maple -> (F2,L2)");
    check(index.postingsText("syrup") == "(2,2)", "after: syrup -> (F2,L2)");
    check(index.postingsText("serve") == "(1,2) (2,2)", "serve is in both texts: unchanged");
    check(index.postingsText("with") == "(1,2) (2,2)", "with is in both texts: unchanged");
}

static void testDeleteUpdatesIndex() {
    FakeIndex index;
    FakeDocs docs;
    buildSpecState(index, docs);
    // vegan.txt deletes "Top with berries." and saves
    indexDeleteLine(index, 2, 4, BERRIES);
    check(!index.hasKey("berries"), "berries key goes when its list empties");
    check(!index.hasKey("top"), "top key goes when its list empties");
    check(index.postingsText("with") == "(1,2) (2,2)", "with keeps its other postings");
}

// a word twice in one line is one posting
static void testRepeatedWordInLine() {
    FakeIndex index;
    indexInsertLine(index, 1, 1, "Honey, honey, HONEY!");
    check(index.postingsText("honey") == "(1,1)", "repeated word gives one posting");
    indexDeleteLine(index, 1, 1, "Honey, honey, HONEY!");
    check(!index.hasKey("honey"), "deleting the line removes it");
}

// Section 3.4: the index follows saved text, the output shows working text
static void testSavedTextOnly() {
    FakeIndex index;
    FakeDocs docs;
    buildSpecState(index, docs);

    // bob edits notes.txt line 1 to say maple, not saved: index and filter untouched
    editWorkingLine(docs.doc(3), 6, "bob", "Maple.");
    checkText(captureSearch("maple", index, docs),
              "vegan.txt:2 [carol] Serve with maple syrup.\n"
              "1 match in 1 document (bloom: 1 of 3 documents passed)\n",
              "unsaved maple is not found");
}

static void testWorkingLineNumberAndText() {
    FakeIndex index;
    FakeDocs docs;
    buildSpecState(index, docs);

    // staged, not saved: a new top line and an edit of the honey line
    addWorkingLine(docs.doc(1), 0, 7, "dave", "Breakfast.");
    editWorkingLine(docs.doc(1), 2, "dave", "Serve with honey and jam.");
    checkText(captureSearch("honey", index, docs),
              "recipes.txt:3 [dave] Serve with honey and jam.\n"
              "1 match in 1 document (bloom: 2 of 3 documents passed)\n",
              "prints the current line number, author and text");
}

static void testStagedDeleteSkipped() {
    FakeIndex index;
    FakeDocs docs;
    buildSpecState(index, docs);
    removeWorkingLine(docs.doc(1), 2);
    checkText(captureSearch("honey", index, docs), "no matches\n",
              "posting of a staged delete is skipped");
}

static void testBinnedDocumentsFiltered() {
    FakeIndex index;
    FakeDocs docs;
    buildSpecState(index, docs);
    docs.doc(1).live = false;   // recipes.txt is in the bin

    checkText(captureSearch("pan*", index, docs),
              "vegan.txt:1 [alice] Pancakes need eggs and flour.\n"
              "1 match in 1 document (bloom: skipped for prefix queries)\n",
              "prefix search skips binned documents");
    checkText(captureSearch("honey", index, docs), "no matches\n",
              "word search skips binned documents");
    checkText(captureSearch("pancakes", index, docs),
              "vegan.txt:1 [alice] Pancakes need eggs and flour.\n"
              "1 match in 1 document (bloom: 1 of 2 documents passed)\n",
              "bloom count is over live documents");
}

static void testAllFiltersReject() {
    FakeIndex index;
    FakeDocs docs;
    buildSpecState(index, docs);
    index.lookups = 0;
    checkText(captureSearch("zebra", index, docs), "no matches\n", "word nobody has");
    check(index.lookups == 0, "B+ tree is skipped when every filter rejects");

    index.lookups = 0;
    captureSearch("honey", index, docs);
    check(index.lookups == 1, "one B+ tree lookup when a filter passes");
}

static void testNoDocuments() {
    FakeIndex index;
    FakeDocs docs;
    checkText(captureSearch("honey", index, docs), "no matches\n", "no documents, word");
    checkText(captureSearch("h*", index, docs), "no matches\n", "no documents, prefix");
}

// several keys under one prefix: merged in FileID, LineID order
static void testPrefixMergesKeys() {
    FakeIndex index;
    FakeDocs docs;
    buildSpecState(index, docs);
    // keys starting with s: serve, served, side, syrup
    checkText(captureSearch("s*", index, docs),
              "recipes.txt:2 [bob] Serve with honey.\n"
              "recipes.txt:3 [alice] Cook five minutes per side.\n"
              "recipes.txt:4 [alice] Best served warm.\n"
              "vegan.txt:2 [carol] Serve with maple syrup.\n"
              "vegan.txt:3 [alice] Cook five minutes per side.\n"
              "5 matches in 2 documents (bloom: skipped for prefix queries)\n",
              "search s*");
    // maple is taken; milk would not be (it is not in these documents anyway)
    checkText(captureSearch("ma*", index, docs),
              "vegan.txt:2 [carol] Serve with maple syrup.\n"
              "1 match in 1 document (bloom: skipped for prefix queries)\n",
              "search ma*");
}

static void testPrefixRemovesDuplicates() {
    FakeIndex index;
    FakeDocs docs;
    FakeDoc& d = docs.add(1, "a.txt");
    addWorkingLine(d, 0, 1, "alice", "Pan, pancake and pancakes.");
    BloomFilter* f = docs.newFilter(nullptr);
    addTokens(f, "Pan, pancake and pancakes.");
    d.head = f;
    indexInsertLine(index, 1, 1, "Pan, pancake and pancakes.");
    checkText(captureSearch("pan*", index, docs),
              "a.txt:1 [alice] Pan, pancake and pancakes.\n"
              "1 match in 1 document (bloom: skipped for prefix queries)\n",
              "a line matching three keys is printed once");
    checkText(captureSearch("pancake", index, docs),
              "a.txt:1 [alice] Pan, pancake and pancakes.\n"
              "1 match in 1 document (bloom: 1 of 1 document passed)\n",
              "exact word does not match longer keys");
}

static void testCaseIsIgnored() {
    FakeIndex index;
    FakeDocs docs;
    buildSpecState(index, docs);
    check(captureSearch("HONEY", index, docs) == captureSearch("honey", index, docs),
          "search HONEY = search honey");
    check(captureSearch("PAN*", index, docs) == captureSearch("pan*", index, docs),
          "search PAN* = search pan*");
}

static void testTermParsing() {
    const char* bad[] = {"", "*", "**", "pan**", "pa*n", "*pan", "hello!", "a-b", "can't", "x y"};
    int badCount = sizeof(bad) / sizeof(bad[0]);
    FakeIndex index;
    FakeDocs docs;
    for (int i = 0; i < badCount; i++) {
        checkText(captureSearch(bad[i], index, docs),
                  "error: search takes one word or a prefix ending in *\n",
                  string("malformed term \"") + bad[i] + "\"");
    }

    string term;
    bool isPrefix = true;
    check(parseSearchTerm("Honey", term, isPrefix) && term == "honey" && !isPrefix, "word is lowercased");
    check(parseSearchTerm("C3", term, isPrefix) && term == "c3" && !isPrefix, "digits are allowed");
    check(parseSearchTerm("p*", term, isPrefix) && term == "p" && isPrefix, "one-character prefix");
}

int main() {
    long long liveBefore = liveAllocations();

    testSpecTranscript();
    testFalsePositiveExplained();
    testEditUpdatesIndex();
    testDeleteUpdatesIndex();
    testRepeatedWordInLine();
    testSavedTextOnly();
    testWorkingLineNumberAndText();
    testStagedDeleteSkipped();
    testBinnedDocumentsFiltered();
    testAllFiltersReject();
    testNoDocuments();
    testPrefixMergesKeys();
    testPrefixRemovesDuplicates();
    testCaseIsIgnored();
    testTermParsing();

    long long leaked = liveAllocations() - liveBefore;
    check(leaked == 0, "no leaked allocations");

    cout << "search: " << passed << " passed, " << failed << " failed" << endl;
    return failed == 0 ? 0 : 1;
}
