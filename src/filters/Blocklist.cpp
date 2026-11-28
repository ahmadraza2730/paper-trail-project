#include "filters/Blocklist.h"
#include "foundation/Hash.h"
#include "foundation/Tokenizer.h"
#include "foundation/InsertionSort.h"
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace std;

Blocklist::Blocklist(int counterCount, int hashCount) {
    m = counterCount;
    k = hashCount;
    avoided = 0;
    for (int i = 0; i < m; i++) {
        counters.add(0);
    }
}

int Blocklist::position(const string& word, int i) const {
    unsigned long long h1 = fnv1a(word);
    unsigned long long h2 = djb2(word) | 1;
    return (int)((h1 + i * h2) % m);
}

//capital letters become lowercase
//small letters and digits are not changed
bool Blocklist::cleanWord(const string& word, string& result) const {
    result = "";
    for (int i = 0; i < (int)word.length(); i++) {
        char c = word[i];
        if (c >= 'A' && c <= 'Z') {
            result += (char)(c - 'A' + 'a');
        }
        else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            result += c;
        }
        else {
            return false;
        }
    }
    return true;
}

void Blocklist::block(const string& word) {
    string clean;
    if (!cleanWord(word, clean)) {
        cout << "error: a blocked word may contain only letters and digits" << endl;
        return;
    }
    if (words.contains(clean)) {
        cout << "error: " << clean << " is already blocked" << endl;
        return;
    }
    words.put(clean, true);
    for (int i = 0; i < k; i++) {
        int pos = position(clean, i);
        if (counters.get(pos) < 15) {
            counters.set(pos, counters.get(pos) + 1);
        }
    }
    cout << "blocked " << clean << endl;
}

void Blocklist::unblock(const string& word) {
    string clean;
    if (!cleanWord(word, clean)) {
        cout << "error: a blocked word may contain only letters and digits" << endl;
        return;
    }
    if (!words.contains(clean)) {
        cout << "error: " << clean << " is not blocked" << endl;
        return;
    }
    words.remove(clean);
    for (int i = 0; i < k; i++) {
        int pos = position(clean, i);
        if (counters.get(pos) < 15) {
            counters.set(pos, counters.get(pos) - 1);
        }
    }
    cout << "unblocked " << clean << endl;
}

bool Blocklist::uncertain(const string& word) const {
    for (int i = 0; i < k; i++) {
        if (counters.get(position(word, i)) == 0) {
            return false;
        }
    }
    return true;
}

bool Blocklist::isBlocked(const string& word) {
    if (!uncertain(word)) {
        avoided++;
        return false;
    }
    return words.contains(word);
}

bool Blocklist::checkLine(int lineNumber, const string& text) {
    DynArray<string> tokens = tokenize(text);
    bool found = false;
    for (int i = 0; i < tokens.size(); i++) {
        if (isBlocked(tokens.get(i))) {
            cout << "warning: line " << lineNumber << " contains blocked word \"" << tokens.get(i) << "\"" << endl;
            found = true;
        }
    }
    return found;
}

bool Blocklist::lineHasBlocked(const string& text) const {
    DynArray<string> tokens = tokenize(text);
    for (int i = 0; i < tokens.size(); i++) {
        if (uncertain(tokens.get(i)) && words.contains(tokens.get(i))) {
            return true;
        }
    }
    return false;
}

static bool alphabetical(const string& a, const string& b) {
    return a < b;
}

void Blocklist::printBlocked() const {
    DynArray<string> list = words.keys();
    insertionSort(list, alphabetical);
    for (int i = 0; i < list.size(); i++) {
        cout << list.get(i) << endl;
    }
    cout << "exact lookups avoided: " << avoided << endl;
}

void Blocklist::runFpTest(int n) {
    for (int i = 0; i < n; i++) {
        string word = "w" + to_string(i);
        for (int j = 0; j < k; j++) {
            int pos = position(word, j);
            if (counters.get(pos) < 15) {
                counters.set(pos, counters.get(pos) + 1);
            }
        }
    }

    int queries = 10000;
    int wrong = 0;
    for (int i = 0; i < queries; i++) {
        if (uncertain("x" + to_string(i))) {
            wrong++;
        }
    }

    int saturated = 0;
    for (int i = 0; i < m; i++) {
        if (counters.get(i) == 15) {
            saturated++;
        }
    }

    double formula = pow(1 - exp(-(double)k * n / m), k) * 100;
    double measured = (double)wrong / queries * 100;

    cout << "fptest counting m=" << m << " k=" << k << " n=" << n << endl;
    cout << fixed << setprecision(2);
    cout << "formula  : " << formula << "%   (1 - e^(-kn/m))^k" << endl;
    cout << "measured : " << measured << "%   (" << wrong << " of "
        << queries << " queries answered \"probably present\")" << endl;
    cout << defaultfloat << setprecision(6);
    cout << "saturated: " << saturated << " counters at 15" << endl;
}
