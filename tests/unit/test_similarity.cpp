// Unit tests for the similarity checker (spec Section 5.10).
// Build and run from the project root with `make unit`, or by hand:
//   g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Isrc tests/unit/test_similarity.cpp
//       src/filters/Similarity.cpp src/filters/BloomFilter.cpp src/foundation/Hash.cpp
//       src/foundation/Tokenizer.cpp src/foundation/AllocTracker.cpp -o test_similarity
//   ./test_similarity

#include "filters/Similarity.h"
#include "foundation/AllocTracker.h"
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

static DynArray<string> linesOf(const char* a, const char* b = nullptr, const char* c = nullptr) {
    DynArray<string> lines;
    lines.add(a);
    if (b != nullptr) {
        lines.add(b);
    }
    if (c != nullptr) {
        lines.add(c);
    }
    return lines;
}

//runs report and returns what it printed
static string captureReport(SimilarityChecker& checker, int k, int percent, CommonAncestorFn lca) {
    ostringstream out;
    streambuf* old = cout.rdbuf(out.rdbuf());
    checker.report(k, percent, lca, nullptr);
    cout.rdbuf(old);
    return out.str();
}

//pretend history: only F3 and F4 share an ancestor, c2
static bool fakeLca(int a, int b, string& commit, void*) {
    if ((a == 3 && b == 4) || (a == 4 && b == 3)) {
        commit = "c2";
        return true;
    }
    return false;
}

static void testShinglesSpanLines() {
    //spec Section 2: "Eggs and flour." + "Add milk."
    DynArray<string> s = makeShingles(linesOf("Eggs and flour.", "Add milk."));
    check(s.size() == 3, "two lines give 3 shingles");
    if (s.size() == 3) {
        check(s.get(0) == "eggs and flour", "shingle 1");
        check(s.get(1) == "and flour add", "shingle 2 spans the line break");
        check(s.get(2) == "flour add milk", "shingle 3");
    }
}

static void testShortAndDuplicateShingles() {
    check(makeShingles(linesOf("Hi there.")).size() == 0, "fewer than 3 tokens: no shingles");
    check(makeShingles(linesOf("")).size() == 0, "empty line: no shingles");
    //"a b a b a b" has shingles "a b a" and "b a b" only
    check(makeShingles(linesOf("a b a b a b")).size() == 2, "repeated shingles counted once");

    SimilarityChecker checker;
    checker.addDocument(1, "tiny.txt", linesOf("Hi."));
    checker.addDocument(2, "big.txt", linesOf("one two three four"));
    check(checker.documentCount() == 1, "document with < 3 tokens is skipped");
}

static void testRounding() {
    Score eighth = {1, 8};
    Score third = {1, 3};
    Score twoThirds = {2, 3};
    Score fiveSevenths = {5, 7};
    Score fourSevenths = {4, 7};
    Score all = {4, 4};
    check(percentOf(eighth) == 13, "12.5% rounds half up to 13%");
    check(percentOf(third) == 33, "1/3 -> 33%");
    check(percentOf(twoThirds) == 67, "2/3 -> 67%");
    check(percentOf(fiveSevenths) == 71, "spec: 5/7 = 71%");
    check(percentOf(fourSevenths) == 57, "spec: 4/7 = 57%");
    check(percentOf(all) == 100, "4/4 = 100%");
}

static SimilarPair pairWith(int hits, int visit) {
    SimilarPair p;
    p.a = 0;
    p.b = 0;
    p.bloom.hits = hits;
    p.bloom.total = 100;
    p.exact = p.bloom;
    p.visit = visit;
    return p;
}

static void testHeapTopK() {
    //spec Section 5.1: scores 0.10, 0.71, 0.05, 0.40, 0.92 with k = 3
    DynArray<SimilarPair> c;
    c.add(pairWith(10, 0));
    c.add(pairWith(71, 1));
    c.add(pairWith(5, 2));
    c.add(pairWith(40, 3));
    c.add(pairWith(92, 4));
    DynArray<SimilarPair> best = topPairs(c, 3);
    check(best.size() == 3, "top 3 has 3 entries");
    if (best.size() == 3) {
        check(best.get(0).bloom.hits == 92, "top-k first is 0.92");
        check(best.get(1).bloom.hits == 71, "top-k second is 0.71");
        check(best.get(2).bloom.hits == 40, "top-k third is 0.40");
    }

    //k larger than the number of pairs
    check(topPairs(c, 50).size() == 5, "k above pair count returns every pair");
}

static void testHeapTies() {
    //equal scores: earlier visits win and are listed first
    DynArray<SimilarPair> c;
    c.add(pairWith(50, 0));
    c.add(pairWith(50, 1));
    c.add(pairWith(50, 2));
    c.add(pairWith(60, 3));
    DynArray<SimilarPair> best = topPairs(c, 3);
    check(best.size() == 3, "tie test has 3 entries");
    if (best.size() == 3) {
        check(best.get(0).visit == 3, "highest score first");
        check(best.get(1).visit == 0, "ties: visit 0 before visit 1");
        check(best.get(2).visit == 1, "ties: visit 2 is dropped (strictly larger rule)");
    }
}

static void testContainmentBothDirections() {
    //spec Section 5.10: a short passage pasted into a long document
    DynArray<string> shortDoc = linesOf("Preheat the oven to 180 degrees.");
    DynArray<string> longDoc = linesOf("Preheat the oven to 180 degrees.",
        "Mix the flour and the eggs.",
        "Pour the batter into the tin and bake for twenty minutes.");
    check(makeShingles(shortDoc).size() == 4, "short passage has 4 shingles");
    check(makeShingles(longDoc).size() == 21, "long document has 21 shingles");

    SimilarityChecker checker;
    checker.addDocument(1, "a.txt", shortDoc);
    checker.addDocument(2, "b.txt", longDoc);
    SimilarPair p = checker.comparePair(0, 1, 0);
    check(p.exact.hits == 4 && p.exact.total == 4, "score is the larger direction, 4/4");
    check(percentOf(p.exact) == 100, "exact containment 100%");
    check(percentOf(p.bloom) == 100, "bloom containment 100%");
}

static void testReportFormat() {
    SimilarityChecker checker;
    //added out of order on purpose: report must use FileID order
    checker.addDocument(3, "recipes.txt", linesOf("Pancakes need eggs and flour.",
        "Serve with honey.", "Cook five minutes per side."));
    checker.addDocument(1, "notes.txt", linesOf("Lecture seven covers heaps and priority queues today."));
    checker.addDocument(4, "vegan.txt", linesOf("Pancakes need flax and flour.",
        "Serve with honey.", "Cook five minutes per side."));
    checker.addDocument(2, "notes_copy.txt", linesOf("Lecture seven covers heaps and priority queues today."));

    //recipes/vegan share 8 of 11 shingles -> 73%
    string expected =
        "rank  A            B               bloom  exact  note\n"
        "1     notes.txt    notes_copy.txt   100%   100%\n"
        "2     recipes.txt  vegan.txt         73%    73%  fork-related (LCA c2)\n";
    checkText(captureReport(checker, 3, 50, fakeLca), expected, "report layout matches the spec transcript");

    //k = 1 keeps only the best pair
    string top1 =
        "rank  A          B               bloom  exact  note\n"
        "1     notes.txt  notes_copy.txt   100%   100%\n";
    checkText(captureReport(checker, 1, 50, fakeLca), top1, "k = 1 keeps the best pair");

    //default threshold 80 drops the 73% pair
    checkText(captureReport(checker, 3, 80, nullptr), top1, "threshold 80 drops the 73% pair");

    checkText(captureReport(checker, 3, 101, nullptr), "no pairs at or above 101%\n", "nothing at 101%");
}

static void testNoPairs() {
    SimilarityChecker none;
    checkText(captureReport(none, 3, 90, nullptr), "no pairs at or above 90%\n", "no documents");

    SimilarityChecker one;
    one.addDocument(1, "a.txt", linesOf("only one document here"));
    checkText(captureReport(one, 3, 0, nullptr), "no pairs at or above 0%\n", "fewer than two documents");

    SimilarityChecker different;
    different.addDocument(1, "a.txt", linesOf("alpha beta gamma delta"));
    different.addDocument(2, "b.txt", linesOf("one two three four"));
    checkText(captureReport(different, 3, 90, nullptr), "no pairs at or above 90%\n", "spec: similar 3 90");
}

static void testThresholdIsInclusive() {
    //A has 4 shingles, 2 of them in B: exactly 50%
    SimilarityChecker checker;
    checker.addDocument(1, "a.txt", linesOf("w1 w2 w3 w4 w5 w6"));
    checker.addDocument(2, "b.txt", linesOf("w1 w2 w3 w4 x1 x2 x3 x4 x5 x6"));
    SimilarPair p = checker.comparePair(0, 1, 0);
    check(p.exact.hits * 2 == p.exact.total, "constructed pair is exactly 50%");
    string out = captureReport(checker, 1, 50, nullptr);
    check(out.find("a.txt") != string::npos, "a score equal to the threshold is reported");
}

static void testBloomNeverBelowExact() {
    //pseudo-random documents from the spec's generator (Section 3.3)
    unsigned long long state = 42;
    const char* words[] = {"red", "blue", "green", "cat", "dog", "sun", "moon", "tea"};
    SimilarityChecker checker;
    for (int d = 0; d < 12; d++) {
        DynArray<string> lines;
        string line = "";
        for (int w = 0; w < 30; w++) {
            state = (state * 1103515245ULL + 12345ULL) % 2147483648ULL;
            line += string(words[(state >> 8) % 8]) + " ";
        }
        lines.add(line);
        checker.addDocument(d + 1, "d" + to_string(d + 1) + ".txt", lines);
    }
    bool ok = true;
    for (int a = 0; a < checker.documentCount(); a++) {
        for (int b = a + 1; b < checker.documentCount(); b++) {
            SimilarPair p = checker.comparePair(a, b, 0);
            //bloom >= exact  <=>  bloom.hits * exact.total >= exact.hits * bloom.total
            if ((long long)p.bloom.hits * p.exact.total < (long long)p.exact.hits * p.bloom.total) {
                ok = false;
            }
        }
    }
    check(ok, "Bloom estimate is never below the exact value");
}

static void testParseArgs() {
    DynArray<string> w;
    int k = 0;
    int pct = 0;
    ostringstream sink;
    streambuf* old = cout.rdbuf(sink.rdbuf());

    w.add("similar");
    w.add("3");
    bool okDefault = parseSimilarArgs(w, k, pct);
    w.add("60");
    bool okGiven = parseSimilarArgs(w, k, pct);
    int givenK = k;
    int givenPct = pct;

    DynArray<string> bad;
    bad.add("similar");
    bad.add("0");
    bool badK = parseSimilarArgs(bad, k, pct);
    bad.set(1, "x");
    bool badWord = parseSimilarArgs(bad, k, pct);
    bad.set(1, "2");
    bad.add("150");
    bool badPct = parseSimilarArgs(bad, k, pct);

    cout.rdbuf(old);
    check(okDefault, "similar 3 parses");
    check(okGiven && givenK == 3 && givenPct == 60, "similar 3 60 parses");
    check(!badK, "k = 0 rejected");
    check(!badWord, "k = x rejected");
    check(!badPct, "percent 150 rejected");

    DynArray<string> d;
    d.add("similar");
    d.add("5");
    parseSimilarArgs(d, k, pct);
    check(pct == 80, "default threshold is 80");
}

int main() {
    long long liveBefore = liveAllocations();

    testShinglesSpanLines();
    testShortAndDuplicateShingles();
    testRounding();
    testHeapTopK();
    testHeapTies();
    testContainmentBothDirections();
    testReportFormat();
    testNoPairs();
    testThresholdIsInclusive();
    testBloomNeverBelowExact();
    testParseArgs();

    long long leaked = liveAllocations() - liveBefore;
    check(leaked == 0, "no leaked allocations");

    cout << "similarity: " << passed << " passed, " << failed << " failed" << endl;
    return failed == 0 ? 0 : 1;
}
