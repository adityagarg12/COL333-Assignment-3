# Makefile for COL333 Assignment 3
# Compiles sat_solver.cpp and solution_decoder.cpp

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
LDFLAGS = 

# Source files
SAT_SOLVER_SRC = sat_solver.cpp
SOLUTION_DECODER_SRC = solution_decoder.cpp

# Executable names
SAT_SOLVER_BIN = sat_solver
SOLUTION_DECODER_BIN = solution_decoder

# Default target - build both executables
all: $(SAT_SOLVER_BIN) $(SOLUTION_DECODER_BIN)

# Build sat_solver executable
$(SAT_SOLVER_BIN): $(SAT_SOLVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Build solution_decoder executable
$(SOLUTION_DECODER_BIN): $(SOLUTION_DECODER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Clean built files
clean:
	rm -f $(SAT_SOLVER_BIN) $(SOLUTION_DECODER_BIN)

# Install (copy to /usr/local/bin - requires sudo)
install: all
	sudo cp $(SAT_SOLVER_BIN) /usr/local/bin/
	sudo cp $(SOLUTION_DECODER_BIN) /usr/local/bin/

# Uninstall (remove from /usr/local/bin - requires sudo)
uninstall:
	sudo rm -f /usr/local/bin/$(SAT_SOLVER_BIN)
	sudo rm -f /usr/local/bin/$(SOLUTION_DECODER_BIN)

# Test compilation (build without optimization for debugging)
debug: CXXFLAGS = -std=c++17 -Wall -Wextra -g -DDEBUG
debug: all

# Force rebuild
rebuild: clean all

# Display help
help:
	@echo "Available targets:"
	@echo "  all          - Build both executables (default)"
	@echo "  sat_solver   - Build only sat_solver executable"
	@echo "  solution_decoder - Build only solution_decoder executable"
	@echo "  clean        - Remove built executables"
	@echo "  debug        - Build with debug flags"
	@echo "  rebuild      - Clean and build"
	@echo "  install      - Install executables to /usr/local/bin"
	@echo "  uninstall    - Remove executables from /usr/local/bin"
	@echo "  help         - Display this help message"

# Phony targets
.PHONY: all clean install uninstall debug rebuild help