// Folder tree, path table, listing, stars, properties and recycle bin (Section 5.2).
// Owner: Hassaan
#pragma once

#include <string>
#include "common/Types.h"
#include "foundation/DynArray.h"
#include "foundation/HashMap.h"

class Document;

struct FsNode {
    std::string     name;
    bool            isFolder;
    FsNode*           parent;
    DynArray<FsNode*> children;     // creation order
    FileID          fileId;       // files only
    Document*       doc;          // files only (Huzaima's Document)
    bool            starred;
    // Recycle bin bookkeeping
    int             binNumber;
    std::string     originalPath;
};

class Explorer {
public:
    Explorer();
    ~Explorer();

    // Commands
    void pwd() const;
    void cd(const std::string& path);
    void ls(const std::string& sortKey, bool descending) const;
    void mkdir(const std::string& name);
    void newDocument(const std::string& name);
    void open(const std::string& name);
    void star(const std::string& name, bool on);
    void props(const std::string& name) const;
    void rm(const std::string& name);
    void bin() const;
    void restore(int binNumber);
    void emptyBin();

    // Used by others
    FsNode*       resolve(const std::string& path) const;   // relative or absolute, nullptr if missing
    std::string absolutePath(const FsNode* node) const;
    FsNode*       currentFolder() const;
    Document*   currentDocument() const;
    int         liveDocumentCount() const;               // includes binned documents (limit 50)

private:
    FsNode* root_;
    FsNode* binRoot_;
    FsNode* cwd_;
    Document* openDoc_;
    HashMap<std::string, FsNode*> pathTable_;
};
