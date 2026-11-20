// Directed graph of documents with counted edges both ways (Sections 5.8, 5.9).
// One instance for links ([[name]]), one for embeds ({{embed:name}}).
// Owner: Ahmad
#pragma once

#include <string>
#include "common/Types.h"
#include "foundation/DynArray.h"
#include "foundation/HashMap.h"

struct Edge {
    FileID other;
    int    count;
};

struct Unresolved {
    std::string target;   // name or absolute path as written
    int         count;
};

class DocGraph {
public:
    void addEdge   (FileID from, FileID to);     // count + 1
    void removeEdge(FileID from, FileID to);     // count - 1, drop at 0
    void removeVertex(FileID file);              // rm: edges gone, links become unresolved

    const DynArray<Edge>* outgoing(FileID file) const;
    const DynArray<Edge>* incoming(FileID file) const;

    // Embed graph only: would u -> v create a cycle? Iterative DFS from v looking for u.
    // On true, fills cyclePath with "a.txt -> b.txt -> a.txt" style FileIDs.
    bool wouldCreateCycle(FileID u, FileID v, DynArray<FileID>& cyclePath) const;

private:
    HashMap<FileID, DynArray<Edge>*>       out_;
    HashMap<FileID, DynArray<Edge>*>       in_;
    HashMap<FileID, DynArray<Unresolved>*> unresolved_;
};

// Commands
struct Engine;
void runBacklinks(Engine& e);
void runRelated  (Engine& e, int hops);   // BFS with RingQueue
void runBroken   (Engine& e);
void runOrphans  (Engine& e);
