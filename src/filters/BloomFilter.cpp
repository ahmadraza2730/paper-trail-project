#include "filters/BloomFilter.h"
#include "foundation/Hash.h"
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace std;

BloomFilter::BloomFilter(int bitCount, int hashCount) {
    m = bitCount;
    k = hashCount;
    bytes = new unsigned char[byteCount()];
    clear();
}

BloomFilter::~BloomFilter() {
    delete[] bytes;
}

BloomFilter::BloomFilter(const BloomFilter& other) {
    m = other.m;
    k = other.k;
    bytes = new unsigned char[byteCount()];
    for (int i = 0; i < byteCount(); i++) {
        bytes[i] = other.bytes[i];
    }
}

BloomFilter& BloomFilter::operator=(const BloomFilter& other) {
    if (this == &other) {
        return *this;
    }
    delete[] bytes;
    m = other.m;
    k = other.k;
    bytes = new unsigned char[byteCount()];
    for (int i = 0; i < byteCount(); i++) {
        bytes[i] = other.bytes[i];
    }
    return *this;
}

int BloomFilter::byteCount() const {
    return (m + 7) / 8;
}

int BloomFilter::position(const string& token, int i) const {
    unsigned long long h1 = fnv1a(token);
    unsigned long long h2 = djb2(token) | 1;
    return (int)((h1 + i * h2) % m);
}

//ticks a box at that position
void BloomFilter::setBit(int pos) {
    bytes[pos / 8] = bytes[pos / 8] | (1 << (pos % 8));
}

bool BloomFilter::getBit(int pos) const {
    return (bytes[pos / 8] & (1 << (pos % 8))) != 0;
}

//ticks the boxes for that word
//positions are got through position()
void BloomFilter::add(const string& token) {
    for (int i = 0; i < k; i++) {
        setBit(position(token, i));
    }
}

//if even a single bit is 1, it will return true.
bool BloomFilter::mightContain(const string& token) const {
    for (int i = 0; i < k; i++) {
        if (!getBit(position(token, i))) {
            return false;
        }
    }
    return true;
}

void BloomFilter::mergeWith(const BloomFilter& other) {
    if (other.m != m) {
        return;
    }
    for (int i = 0; i < byteCount(); i++) {
        bytes[i] = bytes[i] | other.bytes[i];
    }
}

//all 8 boxes to 0 at once
void BloomFilter::clear() {
    for (int i = 0; i < byteCount(); i++) {
        bytes[i] = 0;
    }
}

int BloomFilter::bitCount() const {
    return m;
}

int BloomFilter::hashCount() const {
    return k;
}

void fpTestBloom(int m, int k, int n) {
    BloomFilter filter(m, k);
    for (int i = 0; i < n; i++) {
        filter.add("w" + to_string(i));
    }

    int queries = 10000;
    int wrong = 0;
    for (int i = 0; i < queries; i++) {
        if (filter.mightContain("x" + to_string(i))) {
            wrong++;
        }
    }

    double formula = pow(1 - exp(-(double)k * n / m), k) * 100;
    double measured = (double)wrong / queries * 100;

    cout << "fptest bloom m=" << m << " k=" << k << " n=" << n << endl;
    cout << fixed << setprecision(2);
    cout << "formula  : " << formula << "%   (1 - e^(-kn/m))^k" << endl;
    cout << "measured : " << measured << "%   (" << wrong << " of " 
        << queries << " queries answered \"probably present\")" << endl;
    cout << defaultfloat << setprecision(6);
}
