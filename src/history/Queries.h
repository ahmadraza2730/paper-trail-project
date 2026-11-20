// History queries over the commit DAG: log, view, common / LCA (Section 5.5).
// Owner: Ahmad
#pragma once

#include <string>
#include "engine/Engine.h"

class Commit;
class Document;

void runLog   (Engine& e, Document* doc);
void runView  (Engine& e, Document* doc, const std::string& commitName);
void runCommon(Engine& e, Document* a, Document* b);

// Lowest common ancestor: deepest commit stamped by both heads, ties -> lowest id.
// Returns nullptr if none. Also used by merge and "similar".
Commit* findLca(Commit* a, Commit* b);

// True if 'ancestor' is reachable from 'descendant' through any parents.
bool isAncestor(Commit* ancestor, Commit* descendant);
