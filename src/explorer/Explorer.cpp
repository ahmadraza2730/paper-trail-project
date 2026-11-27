#include "explorer/Explorer.h"
#include "foundation/InsertionSort.h"
#include <iostream>
#include <iomanip>

using namespace std;

Explorer::Explorer() {
    root = new Node;
    root->name = "root";
    root->isFolder = true;
    root->parent = nullptr;
    root->fileId = 0;
    root->lines = 0;
    root->size = 0;
    root->modified = 0;
    root->starred = false;
    current = root;
    openDoc = nullptr;
    paths.put("/", root);
    nextFileId = 1;
    documentCount = 0;
}

Explorer::~Explorer() {
    freeNode(root);
}

void Explorer::freeNode(Node* node) {
    for (int i = 0; i < node->children.size(); i++) {
        freeNode(node->children.get(i));
    }
    delete node;
}

Node* Explorer::findChild(Node* folder, const string& name) {
    for (int i = 0; i < folder->children.size(); i++) {
        if (folder->children.get(i)->name == name) {
            return folder->children.get(i);
        }
    }
    return nullptr;
}

string Explorer::fullPath(Node* node) {
    if (node == root) {
        return "/";
    }
    string path = "";
    while (node != root) {
        path = "/" + node->name + path;
        node = node->parent;
    }
    return path;
}

bool Explorer::cd(const string& path) {
    Node* node = current;
    int start = 0;
    if (!path.empty() && path[0] == '/') {
        node = root;
        start = 1;
    }
    string part = "";
    for (int i = start; i <= (int)path.length(); i++) {
        if (i < (int)path.length() && path[i] != '/') {
            part += path[i];
            continue;
        }
        //move one level up
        if (part == "..") {
            if (node == root) {
                cout << "error: already at the root" << endl;
                return false;
            }
            node = node->parent;
        }

        else if (part != "") {
            Node* child = findChild(node, part);
            
            //not found
            if (child == nullptr) {
                cout << "error: " << part << " does not exist" << endl;
                return false;
            }

            //part is a file
            if (!child->isFolder) {
                cout << "error: " << part << " is not a folder" << endl;
                return false;
            }
            node = child;
        }
        part = "";
    }
    openDoc = nullptr;
    current = node;
    return true;
}

void Explorer::pwd() {
    cout << fullPath(current) << endl;
}

string Explorer::breadcrumb() {
    DynArray<string> names;
    if (openDoc != nullptr) {
        names.add(openDoc->name);
    }
    Node* node = current;
    while (node != nullptr) {
        names.add(node->name);
        node = node->parent;
    }
    string full = "";
    for (int i = names.size() - 1; i >= 0; i--) {
        full += names.get(i);
        if (i > 0) {
            full += " > ";
        }
    }
    if ((int)full.length() <= 40) {
        return full;
    }
    //if name is bigger than 40 characters
    //make it smaller
    string tail = names.get(0);
    for (int i = 1; i < names.size() - 1; i++) {
        string longer = names.get(i) + " > " + tail;
        if ((int)("root > ... > " + longer).length() > 40) {
            break;
        }
        tail = longer;
    }
    return "root > ... > " + tail;
}

bool Explorer::validName(const string& name) {
    if (name.length() < 1 || name.length() > 32 || name == "..") {
        return false;
    }
    for (int i = 0; i < (int)name.length(); i++) {
        char c = name[i];
        bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        bool digit = c >= '0' && c <= '9';
        if (!letter && !digit && c != '.' && c != '_' && c != '-') {
            return false;
        }
    }
    return true;
}

Node* Explorer::addNode(const string& name, bool isFolder) {
    if (!validName(name)) {
        cout << "error: invalid name \"" << name << "\"" << endl;
        return nullptr;
    }
    if (findChild(current, name) != nullptr) {
        cout << "error: " << name << " already exists in " << fullPath(current) << endl;
        return nullptr;
    }
    Node* node = new Node;
    node->name = name;
    node->isFolder = isFolder;
    node->parent = current;
    node->fileId = 0;
    node->lines = 0;
    node->size = 0;
    node->modified = 0;
    node->starred = false;
    current->children.add(node);
    paths.put(fullPath(node), node);
    return node;
}

bool Explorer::mkdir(const string& name) {
    return addNode(name, true) != nullptr;
}

bool Explorer::newFile(const string& name, int tick) {
    if (documentCount == 50) {
        cout << "error: limit of 50 documents reached" << endl;
        return false;
    }
    Node* node = addNode(name, false);
    if (node == nullptr) {
        return false;
    }
    node->fileId = nextFileId;
    node->modified = tick;
    nextFileId++;
    documentCount++;
    cout << "created " << name << " (F" << node->fileId << ")" << endl;
    return true;
}

bool Explorer::star(const string& name, bool on) {
    Node* node = findChild(current, name);
    if (node == nullptr) {
        cout << "error: " << name << " does not exist" << endl;
        return false;
    }
    if (node->isFolder) {
        cout << "error: only files can be starred" << endl;
        return false;
    }
    if (on && node->starred) {
        cout << "error: " << name << " is already starred" << endl;
        return false;
    }
    if (!on && !node->starred) {
        cout << "error: " << name << " is not starred" << endl;
        return false;
    }
    node->starred = on;
    return true;
}

static string sortKey = "name";
static bool descending = false;

static string extensionOf(const string& name) {
    int dot = (int)name.rfind('.');
    if (dot < 0) {
        return "";
    }
    return name.substr(dot + 1);
}

static int compareNodes(Node* a, Node* b) {
    if (sortKey == "modified") {
        return a->modified - b->modified;
    }
    if (sortKey == "size") {
        return a->size - b->size;
    }
    if (sortKey == "ext") {
        return extensionOf(a->name).compare(extensionOf(b->name));
    }
    return a->name.compare(b->name);
}

static bool comesBefore(Node* const& a, Node* const& b) {
    int result = compareNodes(a, b);
    if (descending) {
        return result > 0;
    }
    return result < 0;
}

static bool folderBefore(Node* const& a, Node* const& b) {
    return a->name < b->name;
}

static void printFile(Node* node) {
    if (node->starred) {
        cout << "* ";
    }
    else {
        cout << "  ";
    }
    cout << left << setw(17) << node->name;
    cout << left << setw(8) << (to_string(node->lines) + " lines");
    cout << right << setw(17) << (to_string(node->size) + " B");
    cout << "      t=" << node->modified << endl;
}

bool Explorer::ls(const string& key, const string& order) {
    if ((key != "name" && key != "modified" && key != "size" && key != "ext") || (order != "asc" && order != "desc")) {
        cout << "error: usage: ls [name|modified|size|ext] [asc|desc]" << endl;
        return false;
    }
    sortKey = key;
    descending = (order == "desc");

    DynArray<Node*> starredFiles;
    DynArray<Node*> folders;
    DynArray<Node*> files;
    for (int i = 0; i < current->children.size(); i++) {
        Node* child = current->children.get(i);
        if (child->isFolder) {
            folders.add(child);
        }
        else if (child->starred) {
            starredFiles.add(child);
        }
        else {
            files.add(child);
        }
    }
    insertionSort(starredFiles, comesBefore);
    insertionSort(folders, folderBefore);
    insertionSort(files, comesBefore);

    for (int i = 0; i < starredFiles.size(); i++) {
        printFile(starredFiles.get(i));
    }
    for (int i = 0; i < folders.size(); i++) {
        cout << "  " << folders.get(i)->name << "/" << endl;
    }
    for (int i = 0; i < files.size(); i++) {
        printFile(files.get(i));
    }
    return true;
}

bool Explorer::props(const string& name) {
    Node* node = findChild(current, name);
    if (node == nullptr) {
        cout << "error: " << name << " does not exist" << endl;
        return false;
    }
    if (node->isFolder) {
        cout << "error: only files have properties" << endl;
        return false;
    }
    string ext = extensionOf(name);
    if (ext == "") {
        ext = "none";
    }
    else {
        ext = "." + ext;
    }
    cout << "name: " << name << endl;
    cout << "extension: " << ext << endl;
    cout << "lines: " << node->lines << endl;
    cout << "size: " << node->size << " bytes" << endl;
    cout << "last modified: t=" << node->modified << endl;
    if (node->starred) {
        cout << "starred: yes" << endl;
    }
    else {
        cout << "starred: no" << endl;
    }
    return true;
}

bool Explorer::open(const string& name) {
    Node* node = findChild(current, name);
    if (node == nullptr) {
        cout << "error: " << name << " does not exist" << endl;
        return false;
    }
    if (node->isFolder) {
        cout << "error: " << name << " is a folder, not a document" << endl;
        return false;
    }
    openDoc = node;
    return true;
}
