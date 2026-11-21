#pragma once

#include <string>

class BloomFilter {
private:
    unsigned char* bytes;
    int m;
    int k;

    int byteCount() const;
    int position(const std::string& token, int i) const;
    void setBit(int pos);
    bool getBit(int pos) const;

public:
    BloomFilter(int bitCount = 32768, int hashCount = 3);
    ~BloomFilter();
    BloomFilter(const BloomFilter& other);
    BloomFilter& operator=(const BloomFilter& other);

    void add(const std::string& token);
    bool mightContain(const std::string& token) const;
    void mergeWith(const BloomFilter& other);
    void clear();

    int bitCount() const;
    int hashCount() const;
};

void fpTestBloom(int m, int k, int n);
