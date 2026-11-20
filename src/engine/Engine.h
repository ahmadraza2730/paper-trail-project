// The Engine holds all global state and the hooks that connect the parts.
// Owner: shared. Change only after telling the team.
//
// Hooks: the explorer (Hassaan) and save/merge (Huzaima) call these at the right
// moments. Each owner fills in their part inside Engine.cpp. Until then they stay empty.
#pragma once

#include "common/Types.h"

class Explorer;      // Hassaan  - folders, files, recycle bin
class Blocklist;     // Hassaan
class Document;      // Huzaima  - one document's text and staging
class CommitDag;     // Huzaima
class SearchIndex;   // Ahmad
class DocGraph;      // Ahmad    - used twice: links and embeds

struct Engine {
    Tick now = 0;              // goes up by one per command line (Section 3.3)
    LineID nextLineID = 1;
    FileID nextFileID = 1;

    Explorer*    explorer  = nullptr;
    Blocklist*   blocklist = nullptr;
    CommitDag*   dag       = nullptr;
    SearchIndex* index     = nullptr;
    DocGraph*    links     = nullptr;
    DocGraph*    embeds    = nullptr;
};

// ---- Hooks (proposed; agree on day 1) ----
void onDocumentCreated (Engine& e, Document* doc);   // new, fork, restore: resolve pending links
void onDocumentRemoved (Engine& e, Document* doc);   // rm: drop from graphs, make links unresolved
void onDocumentRestored(Engine& e, Document* doc);   // restore: re-scan saved text
void onDocumentFreed   (Engine& e, Document* doc);   // emptybin: free postings, commit refs
void onOpsSaved        (Engine& e, Document* doc, const Op* ops, int count); // save/merge: index + graphs
