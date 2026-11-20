# PaperTrail build
# Usage: make         -> builds ./papertrail
#        make clean   -> removes build output

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Werror -g -Isrc
TARGET   := papertrail

SRCS := $(shell find src -name '*.cpp')
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build $(TARGET)

-include $(OBJS:.o=.d)

.PHONY: all clean
