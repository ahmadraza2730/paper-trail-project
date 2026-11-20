// Commits, the DAG, reference counting and cascading free (Section 5.5).
// Owner: Huzaima
#pragma once

#include "common/Types.h"
#include "filters/BloomFilter.h"
#include "foundation/HashMap.h"
#include "foundation/RingQueue.h"

class Commit {
public:
    CommitID      id;
    Commit*       parent1;
    Commit*       parent2;     // nullptr unless a merge commit
    Tick          tick;
    int           depth;       // 1 + larger parent depth; root = 0
    RingQueue<Op> ops;         // immutable after save; read with get(i)
    BloomFilter   filter;      // tokens of the document as of this commit
    int           refCount;    // child commits + document heads
    unsigned      stamp;       // per-traversal stamp for LCA (Ahmad)
    unsigned      stamp2;
};

class CommitDag {
public:
    CommitDag();
    ~CommitDag();

    Commit* createRoot(Tick tick);
    Commit* createCommit(Commit* parent, Tick tick, const RingQueue<Op>& ops);
    Commit* createMerge(Commit* p1, Commit* p2, Tick tick, const RingQueue<Op>& ops);

    void    addRef(Commit* c);
    void    release(Commit* c);              // frees at 0 and cascades to parents
    Commit* find(CommitID id) const;

private:
    HashMap<CommitID, Commit*> byId_;
    CommitID nextId_;
};
