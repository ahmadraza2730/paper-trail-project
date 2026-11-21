#include "foundation/Tokenizer.h"

using namespace std;

static bool isLetterOrDigit(char c) {
    if (c >= 'a' && c <= 'z') {
        return true;
    }
    if (c >= 'A' && c <= 'Z') {
        return true;
    }
    if (c >= '0' && c <= '9') {
        return true;
    }
    return false;
}

static char toLower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 'a';
    }
    return c;
}

DynArray<string> tokenize(const string& text) {
    DynArray<string> tokens;
    string current = "";
    for (int i = 0; i < (int)text.length(); i++) {
        char c = text[i];
        //if letter, then add to current
        if (isLetterOrDigit(c)) {
            current += toLower(c);
        }
        //if character is a separater,
        //send the whole word in current to token
        else if (current != "") {
            tokens.add(current);
            current = "";
        }
    }
    if (current != "") {
        tokens.add(current);
    }
    return tokens;
}
