#pragma once

#include <string>
#include "foundation/DynArray.h"
#include "filters/BloomFilter.h"

// Search (spec Section 5.7): `search <word>` and `search <prefix>*`.
//
// How it fits together:
//   1. The index (Hassaan's B+ tree) maps each token to a sorted posting list
//      of (FileID, LineID). It follows SAVED text: the indexInsertLine /
//      indexDeleteLine / indexEditLine helpers below are called for each saved
//      op on save and merge, so unsaved edits are never searchable.
//   2. `search <word>`: ask the head commit's Bloom filter of every live
//      document first. If all of them say no, print "no matches" without
//      touching the B+ tree. Otherwise look the word up once and keep the
//      postings of documents whose filter said yes.
//   3. `search <prefix>*`: filters cannot answer prefixes, so the B+ tree
//      walks its leaves from the first key >= prefix while keys still start
//      with it. The posting lists of those keys are merged, dropping duplicates.
//   4. Each posting is printed with its CURRENT line number from the working
//      text. A posting whose line is no longer in the working text (a staged
//      delete) is skipped.
//
// Search does not own the index or the documents. It talks to them through
// the two small interfaces below, so the B+ tree and the document / commit
// code plug in without search changing.

// One (FileID, LineID) pair. Lists are sorted by file, then line.
struct SearchPosting {
    int file;
    int line;
};

// Called once per key during a prefix scan, in leaf (alphabetical) order.
typedef void (*PostingListFn)(const DynArray<SearchPosting>& postings, void* context);

// What search needs from the B+ tree index.
class WordIndex {
public:
    virtual ~WordIndex() {}

    // Adds (file, line) to token's list, keeping it sorted. Adding a pair
    // that is already there does nothing.
    virtual void addPosting(const std::string& token, int file, int line) = 0;

    // Removes (file, line) from token's list. The key goes when its list empties.
    virtual void removePosting(const std::string& token, int file, int line) = 0;

    // The sorted posting list of exactly this token, or nullptr if absent.
    virtual const DynArray<SearchPosting>* find(const std::string& token) const = 0;

    // Leaf range scan: calls fn for every key that starts with prefix.
    virtual void scanPrefix(const std::string& prefix, PostingListFn fn, void* context) const = 0;
};

// What search needs from the documents and their commits.
class SearchDocuments {
public:
    virtual ~SearchDocuments() {}

    // Live documents (not in the bin), in FileID order.
    virtual int liveCount() const = 0;
    virtual int liveFileId(int i) const = 0;

    // Bloom filter of the document's head commit. nullptr counts as "maybe".
    virtual const BloomFilter* headFilter(int file) const = 0;

    virtual std::string name(int file) const = 0;

    // Looks a line up in the working text. Returns false if the line is not
    // there (for example a staged delete); otherwise sets its current line
    // number (rank), author and text.
    virtual bool workingLine(int file, int line, int& number, std::string& author,
                             std::string& text) const = 0;
};

// Keeping the index current (Section 5.7.1). Call these for each saved op.
// InsertLine: add (file, line) to every token of the text.
void indexInsertLine(WordIndex& index, int file, int line, const std::string& text);
// DeleteLine: remove (file, line) from every token of the old text.
void indexDeleteLine(WordIndex& index, int file, int line, const std::string& oldText);
// EditLine: remove from tokens only in the old text, add to tokens only in the new text.
void indexEditLine(WordIndex& index, int file, int line, const std::string& oldText,
                   const std::string& newText);

// Checks a search term. A word is letters and digits; a prefix is at least one
// letter or digit followed by a single *. Sets term (lowercased, without the *)
// and isPrefix. Returns false for anything else. Prints nothing.
bool parseSearchTerm(const std::string& raw, std::string& term, bool& isPrefix);

// Runs `search <raw>` and prints the result (or the error).
void runSearch(const std::string& raw, const WordIndex& index, const SearchDocuments& docs);
