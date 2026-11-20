// One document: text (AVL tree), LineID map, staging queue, head commit (Sections 5.3, 5.4).
// Owner: Huzaima
#pragma once

#include <string>
#include "common/Types.h"
#include "document/AvlTree.h"
#include "foundation/HashMap.h"
#include "foundation/RingQueue.h"

class Commit;
struct Engine;

class Document {
public:
    Document(FileID id, Commit* rootCommit);
    ~Document();

    // Editing commands. Each prints "staged <Op>(<LineID>)" or an error.
    void add   (Engine& e, const std::string& author, const std::string& text);
    void insert(Engine& e, const std::string& author, int n, const std::string& text);
    void edit  (Engine& e, const std::string& author, int n, const std::string& text);
    void remove(Engine& e, const std::string& author, int n);

    void save   (Engine& e);
    void discard(Engine& e);
    void show   (Engine& e) const;
    void authors() const;
    void tac    () const;

    // Apply / undo one op on the working text (used by discard, view, merge).
    void applyOp  (const Op& op);
    void applyInverse(const Op& op);

    // Used by the explorer (Hassaan)
    int     lineCount() const;
    int     sizeInBytes() const;        // sum of line lengths + one per line
    Tick    lastModified() const;
    bool    hasStagedChanges() const;

    FileID  id() const;
    Commit* head() const;

private:
    FileID                      id_;
    AvlTree                     text_;
    HashMap<LineID, LineNode*>  lineMap_;
    RingQueue<Op>               staged_;
    Commit*                     head_;
    Tick                        lastModified_;
};
