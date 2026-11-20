// Splits a line into tokens (Section 2): maximal runs of A-Z, a-z, 0-9, lowercased.
// Everything else separates. Repeats are kept.
// Owner: Hassaan
#pragma once

#include <string>
#include "foundation/DynArray.h"

void tokenize(const std::string& line, DynArray<std::string>& out);   // appends to out

// True if every character is a letter or digit (used by search and block).
bool isWord(const std::string& s);
