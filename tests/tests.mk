# Headless core test build — NO raylib. (ADR-0001, ADR-0005)
# Invoke from the project root:  make -f tests/tests.mk
# The bare `Makefile` in .gitignore matches at any depth, so this is named
# tests.mk on purpose (verified NOT ignored).
#
# Builds only src/core/*.cpp + tests/*.cpp with g++ and runs the result headlessly.

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wshadow -g -O0 -I src -I tests
OUTDIR   := tests/bin
BIN      := $(OUTDIR)/core_tests

CORE_SRC := $(wildcard src/core/*.cpp)
# juice.cpp is app-layer but has NO raylib dependency (only core/constants.hpp),
# so its pure shake/particle math is unit-tested here too.
APP_PURE := src/app/juice.cpp
TEST_SRC := $(wildcard tests/*.cpp)
SRC      := $(CORE_SRC) $(APP_PURE) $(TEST_SRC)

.PHONY: test clean
test: $(BIN)
	@echo "== running core tests =="
	@./$(BIN)

$(BIN): $(SRC) $(wildcard src/core/*.hpp) tests/doctest.h | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

$(OUTDIR):
	@mkdir -p $(OUTDIR)

clean:
	@rm -rf $(OUTDIR)
