#include "console/Console.h"
#include "foundation/AllocTracker.h"

using namespace std;

int main(int argc, char* argv[]) {
    Console console;
    if (argc > 1) {
        console.runScript(argv[1]);
    }
    else {
        console.run();
    }
    return printAllocReport();
}
