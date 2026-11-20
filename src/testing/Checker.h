// Invariant checker (Section 5.12): "check on|off".
// Ahmad builds the framework; each owner adds checks for their own structures.
// Owner: framework Ahmad, checks by each owner
#pragma once

#include <string>
#include "engine/Engine.h"

class Checker {
public:
    void setEnabled(bool on);            // "check off" prints "invariants: all ok (checked after N commands)"
    bool enabled() const;

    // Runs after every command while enabled. Returns false and prints
    // "INVARIANT FAILED: <name>: <detail>" on the first failure.
    bool runAll(Engine& engine);

private:
    bool enabled_ = false;
    long long commandsChecked_ = 0;
};

// Each owner implements one of these and returns false + fills name/detail on failure.
bool checkExplorer (Engine& e, std::string& name, std::string& detail);  // Hassaan
bool checkBlocklist(Engine& e, std::string& name, std::string& detail);  // Hassaan
bool checkDocuments(Engine& e, std::string& name, std::string& detail);  // Huzaima: AVL + LineID map
bool checkDag      (Engine& e, std::string& name, std::string& detail);  // Huzaima
bool checkIndex    (Engine& e, std::string& name, std::string& detail);  // Ahmad: B+ tree + Bloom
bool checkGraphs   (Engine& e, std::string& name, std::string& detail);  // Ahmad
