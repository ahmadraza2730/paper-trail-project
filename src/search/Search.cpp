#include "search/Search.h"
#include "foundation/Tokenizer.h"
#include <iostream>

using namespace std;

// ---------- keeping the index current ----------

static bool hasToken(const DynArray<string>& tokens, const string& token) {
    for (int i = 0; i < tokens.size(); i++) {
        if (tokens.get(i) == token) {
            return true;
        }
    }
    return false;
}

// tokens of a line with repeats removed, in first-seen order
// (a line has at most 200 characters, so a linear check is enough)
static DynArray<string> distinctTokens(const string& text) {
    DynArray<string> all = tokenize(text);
    DynArray<string> distinct;
    for (int i = 0; i < all.size(); i++) {
        if (!hasToken(distinct, all.get(i))) {
            distinct.add(all.get(i));
        }
    }
    return distinct;
}

void indexInsertLine(WordIndex& index, int file, int line, const string& text) {
    DynArray<string> tokens = distinctTokens(text);
    for (int i = 0; i < tokens.size(); i++) {
        index.addPosting(tokens.get(i), file, line);
    }
}

void indexDeleteLine(WordIndex& index, int file, int line, const string& oldText) {
    DynArray<string> tokens = distinctTokens(oldText);
    for (int i = 0; i < tokens.size(); i++) {
        index.removePosting(tokens.get(i), file, line);
    }
}

void indexEditLine(WordIndex& index, int file, int line, const string& oldText,
                   const string& newText) {
    DynArray<string> oldTokens = distinctTokens(oldText);
    DynArray<string> newTokens = distinctTokens(newText);
    //only in the old text: the line no longer has this word
    for (int i = 0; i < oldTokens.size(); i++) {
        if (!hasToken(newTokens, oldTokens.get(i))) {
            index.removePosting(oldTokens.get(i), file, line);
        }
    }
    //only in the new text: the line gained this word
    for (int i = 0; i < newTokens.size(); i++) {
        if (!hasToken(oldTokens, newTokens.get(i))) {
            index.addPosting(newTokens.get(i), file, line);
        }
    }
    //tokens in both are left alone
}

// ---------- the search term ----------

static bool isLetterOrDigit(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static char toLower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

bool parseSearchTerm(const string& raw, string& term, bool& isPrefix) {
    int n = (int)raw.length();
    isPrefix = n > 0 && raw[n - 1] == '*';
    int end = isPrefix ? n - 1 : n;
    //a word or a prefix needs at least one character
    if (end == 0) {
        return false;
    }
    string lowered = "";
    for (int i = 0; i < end; i++) {
        if (!isLetterOrDigit(raw[i])) {
            return false;
        }
        lowered += toLower(raw[i]);
    }
    term = lowered;
    return true;
}

// ---------- running a search ----------

static bool comesBefore(const SearchPosting& a, const SearchPosting& b) {
    if (a.file != b.file) {
        return a.file < b.file;
    }
    return a.line < b.line;
}

static bool samePosting(const SearchPosting& a, const SearchPosting& b) {
    return a.file == b.file && a.line == b.line;
}

// merges a sorted list into a sorted result, dropping duplicates
static void mergeInto(DynArray<SearchPosting>& result, const DynArray<SearchPosting>& list) {
    DynArray<SearchPosting> merged;
    int i = 0;
    int j = 0;
    while (i < result.size() || j < list.size()) {
        SearchPosting next = {0, 0};
        if (j == list.size() || (i < result.size() && !comesBefore(list.get(j), result.get(i)))) {
            next = result.get(i);
            i++;
        } else {
            next = list.get(j);
            j++;
        }
        if (merged.isEmpty() || !samePosting(merged.get(merged.size() - 1), next)) {
            merged.add(next);
        }
    }
    result = merged;
}

// callback for the prefix scan: context is the result list
static void collectPrefixList(const DynArray<SearchPosting>& postings, void* context) {
    DynArray<SearchPosting>* result = (DynArray<SearchPosting>*)context;
    mergeInto(*result, postings);
}

static string plural(int count, const string& word, const string& words) {
    return count == 1 ? word : words;
}

// Prints the postings whose file is in allowed (both lists sorted by FileID),
// then the summary line. bloomNote is the text inside "(bloom: ...)".
static void printMatches(const DynArray<SearchPosting>& postings, const DynArray<int>& allowed,
                         const SearchDocuments& docs, const string& bloomNote) {
    int matches = 0;
    int documents = 0;
    int lastFile = -1;
    int a = 0;
    for (int i = 0; i < postings.size(); i++) {
        const SearchPosting& p = postings.get(i);
        //both lists are in FileID order, so walk allowed forward alongside
        while (a < allowed.size() && allowed.get(a) < p.file) {
            a++;
        }
        if (a == allowed.size() || allowed.get(a) != p.file) {
            continue;
        }
        int number = 0;
        string author;
        string text;
        //the index follows saved text; skip lines gone from the working text
        if (!docs.workingLine(p.file, p.line, number, author, text)) {
            continue;
        }
        cout << docs.name(p.file) << ":" << number << " [" << author << "] " << text << endl;
        matches++;
        if (p.file != lastFile) {
            documents++;
            lastFile = p.file;
        }
    }
    if (matches == 0) {
        cout << "no matches" << endl;
        return;
    }
    cout << matches << plural(matches, " match", " matches") << " in " << documents
         << plural(documents, " document", " documents") << " (bloom: " << bloomNote << ")" << endl;
}

static void searchWord(const string& word, const WordIndex& index, const SearchDocuments& docs) {
    //(1) ask the head filter of every live document
    DynArray<int> passed;
    int live = docs.liveCount();
    for (int i = 0; i < live; i++) {
        int file = docs.liveFileId(i);
        const BloomFilter* filter = docs.headFilter(file);
        if (filter == nullptr || filter->mightContain(word)) {
            passed.add(file);
        }
    }
    if (passed.isEmpty()) {
        //every document rejected the word: the B+ tree is not touched
        cout << "no matches" << endl;
        return;
    }
    //(2) one B+ tree lookup, keeping documents whose filter passed
    const DynArray<SearchPosting>* found = index.find(word);
    if (found == nullptr) {
        cout << "no matches" << endl;
        return;
    }
    string note = to_string(passed.size()) + " of " + to_string(live) +
                  plural(live, " document", " documents") + " passed";
    //(3) print in FileID, LineID order with current line numbers
    printMatches(*found, passed, docs, note);
}

static void searchPrefix(const string& prefix, const WordIndex& index, const SearchDocuments& docs) {
    //filters cannot answer "starts with", so every live document takes part
    DynArray<int> live;
    for (int i = 0; i < docs.liveCount(); i++) {
        live.add(docs.liveFileId(i));
    }
    DynArray<SearchPosting> postings;
    index.scanPrefix(prefix, collectPrefixList, &postings);
    printMatches(postings, live, docs, "skipped for prefix queries");
}

void runSearch(const string& raw, const WordIndex& index, const SearchDocuments& docs) {
    string term;
    bool isPrefix = false;
    if (!parseSearchTerm(raw, term, isPrefix)) {
        cout << "error: search takes one word or a prefix ending in *" << endl;
        return;
    }
    if (isPrefix) {
        searchPrefix(term, index, docs);
    } else {
        searchWord(term, index, docs);
    }
}
