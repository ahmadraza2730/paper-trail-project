#include "foundation/Hash.h"

using namespace std;

unsigned int hashInt(unsigned int id) {
    return id * 2654435761u;
}

unsigned int fnv1a(const string& text) {
    unsigned int h = 2166136261u;
    for (int i = 0; i < (int)text.length(); i++) {
        h = h ^ (unsigned char)text[i];
        h = h * 16777619u;
    }
    return h;
}

unsigned int djb2(const string& text) {
    unsigned int h = 5381;
    for (int i = 0; i < (int)text.length(); i++) {
        h = h * 33 + (unsigned char)text[i];
    }
    return h;
}
