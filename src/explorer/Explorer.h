#pragma once

#include <string>
#include "foundation/DynArray.h"
#include "foundation/HashMap.h"

struct Node {
    std::string name;
    bool isFolder;
    Node* parent;
    DynArray<Node*> children;
    int fileId;
    int lines;
    int size;
    int modified;
    bool starred;
};

class Explorer {
private:
    Node* root;
    Node* current;
    Node* openDoc;
    HashMap<std::string, Node*> paths;
    int nextFileId;
    int documentCount;
    void freeNode(Node* node);
    bool validName(const std::string& name);
    Node* addNode(const std::string& name, bool isFolder);

public:
    Explorer();
    ~Explorer();
    Node* findChild(Node* folder, const std::string& name);
    std::string fullPath(Node* node);
    bool cd(const std::string& path);
    void pwd();
    std::string breadcrumb();
    bool mkdir(const std::string& name);
    bool newFile(const std::string& name, int tick);
    bool star(const std::string& name, bool on);
    bool ls(const std::string& key, const std::string& order);
    bool props(const std::string& name);
    bool open(const std::string& name);
};
