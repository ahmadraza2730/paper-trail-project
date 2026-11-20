// Reads commands (keyboard or script), parses them and calls the right part.
// Also owns tick counting and the exact error wording of Section 5.13.
// Owner: Hassaan
#pragma once

#include <string>
#include "engine/Engine.h"

class Console {
public:
    explicit Console(Engine& engine);

    int  runInteractive();                         // prompt loop until exit / EOF
    int  runScript(const std::string& path, bool exitAfter);  // papertrail file / run file
    bool execute(const std::string& line);         // one command line; false on "exit"

    // Merge asks for keep / take / abort through this (keyboard or next script line).
    std::string readAnswerLine();

    std::string prompt() const;                    // breadcrumb + " $ " (Section 5.2.2)

private:
    Engine& engine_;
};

// Parsing helpers others may use:
// Parses "text" with \" escapes. Returns false and sets error on bad input.
bool parseQuoted(const std::string& raw, std::string& out, std::string& error);
