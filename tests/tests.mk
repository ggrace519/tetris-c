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

# Source under coverage (our code only — used to filter the gcov report).
COV_SRC := $(CORE_SRC) $(APP_PURE)

.PHONY: test coverage clean
test: $(BIN)
	@echo "== running core tests =="
	@./$(BIN)

$(BIN): $(SRC) $(wildcard src/core/*.hpp) tests/doctest.h | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

# Compile with gcov instrumentation, run, and report line coverage for our
# source files only. Object/gcov data live in $(OUTDIR) so the tree stays clean.
# All sources compile into one binary, so gcov data files are named
# "cov_tests-<base>.gcno"; run gcov on those prefixed files (in $(OUTDIR)) and
# keep only the report lines for our own src/ files (not doctest/stdlib headers).
coverage: | $(OUTDIR)
	@echo "== building instrumented tests =="
	@$(CXX) $(CXXFLAGS) --coverage -o $(OUTDIR)/cov_tests $(SRC)
	@echo "== running =="
	@cd $(OUTDIR) && ./cov_tests > /dev/null
	@echo "== line coverage (our source only) =="
	@cd $(OUTDIR) && gcov -r cov_tests-*.gcno 2>/dev/null \
	  | grep -A1 -E "File 'src/(core|app)/" \
	  | paste - - - \
	  | sed -E "s/File '//; s/'//; s/Lines executed://; s/of [0-9]+//; s/--//g" \
	  | awk '{printf "  %-26s %s\n", $$1, $$2}' \
	  | sort || true
	@rm -f $(OUTDIR)/*.gcov

$(OUTDIR):
	@mkdir -p $(OUTDIR)

clean:
	@rm -rf $(OUTDIR)
	@rm -f *.gcov
