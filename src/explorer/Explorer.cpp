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
    root->binNumber = 0;
    bin = new Node;
    bin->name = "Recycle Bin";
    bin->isFolder = true;
    bin->parent = nullptr;
    bin->fileId = 0;
    bin->lines = 0;
    bin->size = 0;
    bin->modified = 0;
    bin->starred = false;
    bin->binNumber = 0;
    current = root;
    openDoc = nullptr;
    paths.put("/", root);
    nextFileId = 1;
    documentCount = 0;
    nextBinNumber = 1;
}

Explorer::~Explorer() {
    freeNode(root);
    freeNode(bin);
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
    node->binNumber = 0;
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

void Explorer::removeChild(Node* folder, Node* child) {
    int index = 0;
    while (folder->children.get(index) != child) {
        index++;
    }
    for (int i = index; i < folder->children.size() - 1; i++) {
        folder->children.set(i, folder->children.get(i + 1));
    }
    folder->children.remove();
}

void Explorer::removePaths(Node* node) {
    paths.remove(fullPath(node));
    for (int i = 0; i < node->children.size(); i++) {
        removePaths(node->children.get(i));
    }
}

int Explorer::countDocuments(Node* node) {
    if (!node->isFolder) {
        return 1;
    }
    int count = 0;
    for (int i = 0; i < node->children.size(); i++) {
        count += countDocuments(node->children.get(i));
    }
    return count;
}

bool Explorer::rm(const string& name) {
    Node* node = findChild(current, name);
    if (node == nullptr) {
        cout << "error: " << name << " does not exist" << endl;
        return false;
    }
    if (openDoc == node) {
        openDoc = nullptr;
    }
    removePaths(node);
    removeChild(current, node);
    node->fromPath = fullPath(current);
    node->binNumber = nextBinNumber;
    nextBinNumber++;
    node->parent = bin;
    bin->children.add(node);
    cout << "moved " << name;
    if (node->isFolder) {
        cout << "/";
    }
    cout << " to the recycle bin as item " << node->binNumber << endl;
    return true;
}

void Explorer::showBin() {
    if (bin->children.isEmpty()) {
        cout << "the bin is empty" << endl;
        return;
    }
    for (int i = 0; i < bin->children.size(); i++) {
        Node* node = bin->children.get(i);
        cout << node->binNumber << " " << node->name;
        if (node->isFolder) {
            cout << "/";
        }
        cout << " from " << node->fromPath;
        if (node->isFolder) {
            int count = countDocuments(node);
            if (count == 1) {
                cout << " (1 document)";
            }
            else {
                cout << " (" << count << " documents)";
            }
        }
        cout << endl;
    }
}

void Explorer::addPaths(Node* node) {
    paths.put(fullPath(node), node);
    for (int i = 0; i < node->children.size(); i++) {
        addPaths(node->children.get(i));
    }
}

string Explorer::restoredName(const string& name) {
    int dot = (int)name.rfind('.');
    if (dot < 0) {
        return name + "_restored";
    }
    return name.substr(0, dot) + "_restored" + name.substr(dot);
}

bool Explorer::restore(const string& number) {
    Node* node = nullptr;
    for (int i = 0; i < bin->children.size(); i++) {
        if (to_string(bin->children.get(i)->binNumber) == number) {
            node = bin->children.get(i);
        }
    }
    if (node == nullptr) {
        cout << "error: the bin has no item " << number << endl;
        return false;
    }
    Node* target = root;
    bool available = false;
    if (paths.contains(node->fromPath) && paths.get(node->fromPath)->isFolder) {
        target = paths.get(node->fromPath);
        available = true;
    }
    string oldName = node->name;
    bool taken = false;
    while (findChild(target, node->name) != nullptr) {
        node->name = restoredName(node->name);
        taken = true;
    }
    removeChild(bin, node);
    node->parent = target;
    node->binNumber = 0;
    target->children.add(node);
    addPaths(node);
    cout << "restored " << fullPath(node);
    if (!available && taken) {
        cout << " (its folder is not available; " << oldName << " was taken)";
    }
    else if (!available) {
        cout << " (its folder " << node->fromPath << " is not available)";
    }
    else if (taken) {
        cout << " (" << oldName << " was taken)";
    }
    cout << endl;
    return true;
}

void Explorer::emptyBin() {
    int items = bin->children.size();
    int documents = 0;
    for (int i = 0; i < items; i++) {
        documents += countDocuments(bin->children.get(i));
        freeNode(bin->children.get(i));
    }
    bin->children.clear();
    documentCount -= documents;
    cout << "emptied the bin: " << items;
    if (items == 1) {
        cout << " item";
    }
    else {
        cout << " items";
    }
    cout << " deleted permanently, " << documents;
    if (documents == 1) {
        cout << " document";
    }
    else {
        cout << " documents";
    }
    cout << " freed" << endl;
}
