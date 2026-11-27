#include "console/Console.h"
#include <iostream>
#include <fstream>

using namespace std;

struct CommandInfo {
    const char* name;
    int minArgs;
    int maxArgs;
    const char* usage;
};

static const CommandInfo commands[] = {
    {"pwd", 0, 0, "pwd"},
    {"cd", 1, 1, "cd <path>"},
    {"ls", 0, 2, "ls [name|modified|size|ext] [asc|desc]"},
    {"mkdir", 1, 1, "mkdir <name>"},
    {"new", 1, 1, "new <name>"},
    {"open", 1, 1, "open <name>"},
    {"star", 1, 1, "star <name>"},
    {"unstar", 1, 1, "unstar <name>"},
    {"props", 1, 1, "props <name>"},
    {"rm", 1, 1, "rm <name>"},
    {"bin", 0, 0, "bin"},
    {"restore", 1, 1, "restore <n>"},
    {"emptybin", 0, 0, "emptybin"},
    {"show", 0, 0, "show"},
    {"authors", 0, 0, "authors"},
    {"tac", 1, 1, "tac <name>"},
    {"save", 0, 0, "save"},
    {"discard", 0, 0, "discard"},
    {"log", 0, 0, "log"},
    {"view", 1, 1, "view <commit>"},
    {"fork", 2, 2, "fork <doc> <new-name>"},
    {"common", 2, 2, "common <doc1> <doc2>"},
    {"merge", 3, 3, "merge <incoming> into <current>"},
    {"search", 1, 1, "search <word>"},
    {"backlinks", 0, 0, "backlinks"},
    {"related", 1, 1, "related <h>"},
    {"broken", 0, 0, "broken"},
    {"orphans", 0, 0, "orphans"},
    {"similar", 1, 2, "similar <k> [percent]"},
    {"block", 1, 1, "block <word>"},
    {"unblock", 1, 1, "unblock <word>"},
    {"blocked", 0, 0, "blocked"},
    {"fptest", 4, 4, "fptest <bloom|counting> <m> <k> <n>"},
    {"check", 1, 1, "check on|off"},
    {"fuzz", 2, 2, "fuzz <seed> <count>"},
    {"run", 1, 1, "run <file>"},
    {"exit", 0, 0, "exit"}
};

static const CommandInfo asCommands[] = {
    {"add", 1, 1, "as <author> add \"text\""},
    {"insert", 2, 2, "as <author> insert <n> \"text\""},
    {"edit", 2, 2, "as <author> edit <n> \"text\""},
    {"delete", 1, 1, "as <author> delete <n>"},
    {"embed", 1, 1, "as <author> embed <name>"}
};

static const int commandCount = sizeof(commands) / sizeof(commands[0]);
static const int asCommandCount = sizeof(asCommands) / sizeof(asCommands[0]);

static int findCommand(const CommandInfo table[], int size, const string& name) {
    for (int i = 0; i < size; i++) {
        if (name == table[i].name) {
            return i;
        }
    }
    return -1;
}

static bool checkUsage(DynArray<string>& words, DynArray<bool>& quoted) {
    string command = words.get(0);
    if (command == "as") {
        if (words.size() < 3) {
            cout << "error: usage: as <author> <command>" << endl;
            return false;
        }

        //getting the action from asCommands
        string action = words.get(2);
        int index = findCommand(asCommands, asCommandCount, action);
        if (index == -1) {
            cout << "error: unknown command \"" << action << "\"" << endl;
            return false;
        }

        //counts the words and compares with min and max number of arguments
        int args = words.size() - 3;
        if (args < asCommands[index].minArgs || args > asCommands[index].maxArgs) {
            cout << "error: usage: " << asCommands[index].usage << endl;
            return false;
        }

        //for add, insert and edit the word needs to be quoted
        if ((action == "add" || action == "insert" || action == "edit") && !quoted.get(words.size() - 1)) {
            cout << "error: text must be inside double quotes" << endl;
            return false;
        }
        return true;
    }
    //same for commands
    int index = findCommand(commands, commandCount, command);
    if (index == -1) {
        cout << "error: unknown command \"" << command << "\"" << endl;
        return false;
    }
    int args = words.size() - 1;
    if (args < commands[index].minArgs || args > commands[index].maxArgs) {
        cout << "error: usage: " << commands[index].usage << endl;
        return false;
    }
    //merge has a fixed word in middle that is into
    if (command == "merge" && words.get(2) != "into") {
        cout << "error: usage: " << commands[index].usage << endl;
        return false;
    }
    return true;
}

static bool isBlank(const string& line) {
    for (int i = 0; i < (int)line.length(); i++) {
        if (line[i] != ' ' && line[i] != '\t') {
            return false;
        }
    }
    return true;
}

Console::Console() {
    running = true;
    ticks = 0;
}

string Console::prompt() {
    return explorer.breadcrumb() + " $ ";
}

bool Console::split(const string& line, DynArray<string>& words, DynArray<bool>& quoted) {
    int i = 0;
    int n = (int)line.length();
    while (i < n) {
        if (line[i] == ' ' || line[i] == '\t') {
            i++;
        }
        else if (line[i] == '"') {
            string text = "";
            bool closed = false;
            i++;
            while (i < n) {
                if (line[i] == '\\' && i + 1 < n && line[i + 1] == '"') {
                    text += '"';
                    i += 2;
                }
                else if (line[i] == '"') {
                    closed = true;
                    i++;
                    break;
                }
                else {
                    text += line[i];
                    i++;
                }
            }
            if (!closed) {
                cout << "error: text must be inside double quotes" << endl;
                return false;
            }
            if ((int)text.length() > 200) {
                cout << "error: line text is longer than 200 characters" << endl;
                return false;
            }
            words.add(text);
            quoted.add(true);
        }
        else {
            string word = "";
            while (i < n && line[i] != ' ' && line[i] != '\t') {
                word += line[i];
                i++;
            }
            words.add(word);
            quoted.add(false);
        }
    }
    return true;
}

void Console::handle(const string& line) {
    if (isBlank(line)) {
        return;
    }
    ticks++;
    DynArray<string> words;
    DynArray<bool> quoted;
    if (!split(line, words, quoted)) {
        return;
    }
    if (words.isEmpty()) {
        return;
    }
    if (!checkUsage(words, quoted)) {
        return;
    }
    string command = words.get(0);
    if (command == "exit") {
        running = false;
    }
    else if (command == "run") {
        runScript(words.get(1));
    }
    else if (command == "cd") {
        explorer.cd(words.get(1));
    }
    else if (command == "pwd") {
        explorer.pwd();
    }
    else if (command == "mkdir") {
        explorer.mkdir(words.get(1));
    }
    else if (command == "new") {
        explorer.newFile(words.get(1), ticks);
    }
    else if (command == "star") {
        explorer.star(words.get(1), true);
    }
    else if (command == "unstar") {
        explorer.star(words.get(1), false);
    }
    else if (command == "ls") {
        string key = "name";
        string order = "asc";
        if (words.size() > 1) {
            key = words.get(1);
        }
        if (words.size() > 2) {
            order = words.get(2);
        }
        explorer.ls(key, order);
    }
    else if (command == "props") {
        explorer.props(words.get(1));
    }
    else if (command == "open") {
        explorer.open(words.get(1));
    }
}

void Console::run() {
    string line;
    while (running) {
        cout << prompt();
        if (!getline(cin, line)) {
            cout << endl;
            break;
        }
        handle(line);
    }
}

void Console::runScript(const string& fileName) {
    ifstream file(fileName);
    if (!file) {
        cout << "error: cannot open " << fileName << endl;
        return;
    }
    string line;
    while (running && getline(file, line)) {
        if (isBlank(line) || line[0] == '#') {
            continue;
        }
        cout << prompt() << line << endl;
        handle(line);
    }
}
