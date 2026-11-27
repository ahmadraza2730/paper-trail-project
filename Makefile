# PaperTrail build
# Usage: make         -> builds ./papertrail
#        make unit    -> builds and runs every tests/unit/test_*.cpp
#        make clean   -> removes build output

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Werror -g -Isrc
LDFLAGS  := -static-libstdc++ -static-libgcc
TARGET   := papertrail

SRCS := $(shell find src -name '*.cpp')
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# each unit test has its own main(), so link it with every object except main.o
UNIT_SRCS := $(wildcard tests/unit/test_*.cpp)
LIB_OBJS  := $(filter-out build/main.o,$(OBJS))

unit: $(LIB_OBJS)
	@mkdir -p build/tests
	@for t in $(UNIT_SRCS); do \
		name=build/tests/$$(basename $$t .cpp); \
		$(CXX) $(CXXFLAGS) -o $$name $$t $(LIB_OBJS) $(LDFLAGS) || exit 1; \
		./$$name || exit 1; \
	done

clean:
	rm -rf build $(TARGET)

-include $(OBJS:.o=.d)

.PHONY: all clean unit
