#include "foundation/AllocTracker.h"
#include <cstdlib>
#include <iostream>

using namespace std;

static long long madeCount = 0;
static long long freedCount = 0;

void* operator new(size_t size) {
    if (size == 0) {
        size = 1;
    }
    void* p = malloc(size);
    if (p != nullptr) {
        madeCount++;
    }
    return p;
}

void* operator new[](size_t size) {
    return operator new(size);
}

void operator delete(void* p) noexcept {
    if (p == nullptr) {
        return;
    }
    freedCount++;
    free(p);
}

void operator delete[](void* p) noexcept {
    operator delete(p);
}

void operator delete(void* p, size_t) noexcept {
    operator delete(p);
}

void operator delete[](void* p, size_t) noexcept {
    operator delete(p);
}

long long allocationsMade() {
    return madeCount;
}

long long allocationsFreed() {
    return freedCount;
}

long long liveAllocations() {
    return madeCount - freedCount;
}

int printAllocReport() {
    cout << "allocations: " << allocationsMade() << " made, " << allocationsFreed() << " freed, " << liveAllocations() << " live" << endl;
    if (liveAllocations() > 0) {
        cout << "LEAK: " << liveAllocations() << " live allocations" << endl;
        return 1;
    }
    return 0;
}
