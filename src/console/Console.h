#pragma once

#include <string>
#include "foundation/DynArray.h"

class Console {
private:
    bool running;
    std::string prompt();
    bool split(const std::string& line, DynArray<std::string>& words, DynArray<bool>& quoted);
    void handle(const std::string& line);

public:
    Console();
    void run();
    void runScript(const std::string& fileName);
};
