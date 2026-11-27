#include "filters/Similarity.h"
#include "foundation/Tokenizer.h"
#include "foundation/MinHeap.h"
#include "foundation/InsertionSort.h"
#include <iostream>

using namespace std;

static const int SHINGLE_WIDTH = 3;   // w = 3 tokens per shingle
static const int BITS_PER_SHINGLE = 10;   // m = 10n
static const int BLOOM_HASHES = 7;   // k = 7

//compares two fractions without dividing
//returns -1 if x < y, 0 if equal, 1 if x > y
static int compareScores(const Score& x, const Score& y) {
    long long left = (long long)x.hits * y.total;
    long long right = (long long)y.hits * x.total;
    if (left < right) {
        return -1;
    }
    if (left > right) {
        return 1;
    }
    return 0;
}

static Score largerScore(const Score& x, const Score& y) {
    if (compareScores(x, y) >= 0) {
        return x;
    }
    return y;
}

bool SimilarPair::operator<(const SimilarPair& other) const {
    int c = compareScores(bloom, other.bloom);
    if (c != 0) {
        return c < 0;
    }
    //same score: the later visit is the smaller one
    return visit > other.visit;
}

DynArray<string> makeShingles(const DynArray<string>& lines) {
    //1. all tokens of the document, in line order
    DynArray<string> tokens;
    for (int i = 0; i < lines.size(); i++) {
        DynArray<string> lineTokens = tokenize(lines.get(i));
        for (int j = 0; j < lineTokens.size(); j++) {
            tokens.add(lineTokens.get(j));
        }
    }

    //2. every run of 3 tokens, keeping only the first copy of each shingle
    DynArray<string> shingles;
    HashMap<string, bool> seen;
    for (int i = 0; i + SHINGLE_WIDTH <= tokens.size(); i++) {
        string shingle = tokens.get(i);
        for (int j = 1; j < SHINGLE_WIDTH; j++) {
            shingle += " " + tokens.get(i + j);
        }
        if (!seen.contains(shingle)) {
            seen.put(shingle, true);
            shingles.add(shingle);
        }
    }
    return shingles;
}

int percentOf(const Score& s) {
    if (s.total == 0) {
        return 0;
    }
    //hits * 100 / total, rounded with halves up:
    //floor((200 * hits + total) / (2 * total))
    long long top = 200LL * s.hits + s.total;
    long long bottom = 2LL * s.total;
    return (int)(top / bottom);
}

DynArray<SimilarPair> topPairs(const DynArray<SimilarPair>& candidates, int k) {
    MinHeap<SimilarPair> heap;
    for (int i = 0; i < candidates.size(); i++) {
        const SimilarPair& pair = candidates.get(i);
        if (heap.size() < k) {
            heap.add(pair);
        }
        //heap is full: the root is the weakest of the best k.
        //replace it only if the new score is strictly larger
        else if (compareScores(pair.bloom, heap.peekMin().bloom) > 0) {
            heap.removeMin();
            heap.add(pair);
        }
    }

    //the heap empties smallest first, so fill the result from the back
    int count = heap.size();
    DynArray<SimilarPair> best;
    for (int i = 0; i < count; i++) {
        best.add(SimilarPair());
    }
    for (int i = count - 1; i >= 0; i--) {
        best.set(i, heap.removeMin());
    }
    return best;
}

//reads a whole number made only of digits (at most 9 of them)
static bool readNumber(const string& text, int& value) {
    if (text.length() == 0 || text.length() > 9) {
        return false;
    }
    value = 0;
    for (int i = 0; i < (int)text.length(); i++) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
        value = value * 10 + (text[i] - '0');
    }
    return true;
}

bool parseSimilarArgs(const DynArray<string>& words, int& k, int& percent) {
    if (words.size() < 2 || words.size() > 3) {
        cout << "error: usage: similar <k> [percent]" << endl;
        return false;
    }
    if (!readNumber(words.get(1), k) || k < 1) {
        cout << "error: k must be a whole number of at least 1" << endl;
        return false;
    }
    percent = 80;
    if (words.size() == 3) {
        if (!readNumber(words.get(2), percent) || percent > 100) {
            cout << "error: percent must be a whole number from 0 to 100" << endl;
            return false;
        }
    }
    return true;
}

SimilarityChecker::SimilarityChecker() {
}

SimilarityChecker::~SimilarityChecker() {
    for (int i = 0; i < docs.size(); i++) {
        delete docs.get(i)->exact;
        delete docs.get(i)->filter;
        delete docs.get(i);
    }
}

void SimilarityChecker::addDocument(int fileId, const string& name, const DynArray<string>& lines) {
    DynArray<string> shingles = makeShingles(lines);
    int n = shingles.size();
    if (n == 0) {
        return;
    }

    SimilarityDoc* doc = new SimilarityDoc;
    doc->fileId = fileId;
    doc->name = name;
    doc->shingles = shingles;
    doc->exact = new HashMap<string, bool>();
    doc->filter = new BloomFilter(BITS_PER_SHINGLE * n, BLOOM_HASHES);
    for (int i = 0; i < n; i++) {
        doc->exact->put(shingles.get(i), true);
        doc->filter->add(shingles.get(i));
    }
    docs.add(doc);
}

int SimilarityChecker::documentCount() const {
    return docs.size();
}

static bool smallerFileId(SimilarityDoc* const& x, SimilarityDoc* const& y) {
    return x->fileId < y->fileId;
}

void SimilarityChecker::sortByFileId() {
    insertionSort(docs, smallerFileId);
}

//containment in both directions, Bloom estimate and exact value
SimilarPair SimilarityChecker::comparePair(int a, int b, int visit) const {
    const SimilarityDoc* docA = docs.get(a);
    const SimilarityDoc* docB = docs.get(b);

    //how much of A appears in B
    Score bloomAB = {0, docA->shingles.size()};
    Score exactAB = {0, docA->shingles.size()};
    for (int i = 0; i < docA->shingles.size(); i++) {
        const string& s = docA->shingles.get(i);
        if (docB->filter->mightContain(s)) {
            bloomAB.hits++;
        }
        if (docB->exact->contains(s)) {
            exactAB.hits++;
        }
    }

    //how much of B appears in A
    Score bloomBA = {0, docB->shingles.size()};
    Score exactBA = {0, docB->shingles.size()};
    for (int i = 0; i < docB->shingles.size(); i++) {
        const string& s = docB->shingles.get(i);
        if (docA->filter->mightContain(s)) {
            bloomBA.hits++;
        }
        if (docA->exact->contains(s)) {
            exactBA.hits++;
        }
    }

    SimilarPair pair;
    pair.a = a;
    pair.b = b;
    pair.bloom = largerScore(bloomAB, bloomBA);
    pair.exact = largerScore(exactAB, exactBA);
    pair.visit = visit;
    return pair;
}

//text padded with spaces on the right up to width
static string padRight(const string& text, int width) {
    string result = text;
    while ((int)result.length() < width) {
        result += " ";
    }
    return result;
}

//text padded with spaces on the left up to width
static string padLeft(const string& text, int width) {
    string result = text;
    while ((int)result.length() < width) {
        result = " " + result;
    }
    return result;
}

void SimilarityChecker::report(int k, int percent, CommonAncestorFn lca, void* context) {
    sortByFileId();

    //visit every pair (A, B) with A before B in FileID order,
    //keeping only the pairs at or above the threshold
    DynArray<SimilarPair> candidates;
    int visit = 0;
    for (int a = 0; a < docs.size(); a++) {
        for (int b = a + 1; b < docs.size(); b++) {
            SimilarPair pair = comparePair(a, b, visit);
            visit++;
            //score >= percent, i.e. hits * 100 >= percent * total
            if ((long long)pair.bloom.hits * 100 >= (long long)percent * pair.bloom.total) {
                candidates.add(pair);
            }
        }
    }

    DynArray<SimilarPair> best = topPairs(candidates, k);
    if (best.size() == 0) {
        cout << "no pairs at or above " << percent << "%" << endl;
        return;
    }

    //each text column is as wide as its longest entry plus two spaces
    int rankWidth = 4;   // "rank"
    int aWidth = 1;   // "A"
    int bWidth = 1;   // "B"
    for (int i = 0; i < best.size(); i++) {
        int rankLength = (int)to_string(i + 1).length();
        int aLength = (int)docs.get(best.get(i).a)->name.length();
        int bLength = (int)docs.get(best.get(i).b)->name.length();
        if (rankLength > rankWidth) {
            rankWidth = rankLength;
        }
        if (aLength > aWidth) {
            aWidth = aLength;
        }
        if (bLength > bWidth) {
            bWidth = bLength;
        }
    }

    cout << padRight("rank", rankWidth + 2) << padRight("A", aWidth + 2)
        << padRight("B", bWidth + 2) << "bloom  exact  note" << endl;

    for (int i = 0; i < best.size(); i++) {
        const SimilarPair& pair = best.get(i);
        const SimilarityDoc* docA = docs.get(pair.a);
        const SimilarityDoc* docB = docs.get(pair.b);

        cout << padRight(to_string(i + 1), rankWidth + 2)
            << padRight(docA->name, aWidth + 2)
            << padRight(docB->name, bWidth + 2)
            << padLeft(to_string(percentOf(pair.bloom)) + "%", 5) << "  "
            << padLeft(to_string(percentOf(pair.exact)) + "%", 5);

        string commit;
        if (lca != nullptr && lca(docA->fileId, docB->fileId, commit, context)) {
            cout << "  fork-related (LCA " << commit << ")";
        }
        cout << endl;
    }
}
