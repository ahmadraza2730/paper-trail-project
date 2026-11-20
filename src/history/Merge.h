// "merge <incoming> into <current>" with conflict resolution (Section 5.6).
// Also "fork <doc> <new-name>" (Section 5.5).
// Owner: Huzaima
#pragma once

#include <string>
#include "engine/Engine.h"

class Console;
class Document;

void runFork (Engine& e, Document* source, const std::string& newName);
void runMerge(Engine& e, Console& console, Document* incoming, Document* target);
