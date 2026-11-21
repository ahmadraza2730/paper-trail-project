#pragma once

#include <string>

unsigned int hashInt(unsigned int id);
unsigned int fnv1a(const std::string& text);
unsigned int djb2(const std::string& text);
