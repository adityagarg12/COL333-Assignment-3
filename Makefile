# Makefile for COL333 Assignment 3
# Compiles sat_solver.cpp and solution_decoder.cpp

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
LDFLAGS = 

# Source files
SAT_SOLVER_SRC = sat_solver.cpp
SOLUTION_DECODER_SRC = solution_decoder.cpp
SAT_SOLVER_GPT_SRC = sat_solver_gpt.cpp
SAT_SOLVER_GPT_BIN = sat_solver_gpt
SOLUTION_DECODER_GPT_SRC = solution_decoder_gpt.cpp
SOLUTION_DECODER_GPT_BIN = solution_decoder_gpt
SOLUTION_DECODER_NEW_SRC = solution_decoder_new.cpp
SOLUTION_DECODER_NEW_BIN = solution_decoder_new
SAT_SOLVER_NEW_SRC = sat_solver_new.cpp
SAT_SOLVER_NEW_BIN = sat_solver_new


# Executable names
SAT_SOLVER_BIN = sat_solver
SOLUTION_DECODER_BIN = solution_decoder

# Default target - build both executables
all: $(SAT_SOLVER_BIN) $(SOLUTION_DECODER_BIN) $(SAT_SOLVER_GPT_BIN) $(SOLUTION_DECODER_GPT_BIN)

# Build solution_decoder_gpt executable
$(SOLUTION_DECODER_GPT_BIN): $(SOLUTION_DECODER_GPT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)


# Build sat_solver_gpt executable
$(SAT_SOLVER_GPT_BIN): $(SAT_SOLVER_GPT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)


# Build sat_solver executable
$(SAT_SOLVER_BIN): $(SAT_SOLVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Build solution_decoder executable
$(SOLUTION_DECODER_BIN): $(SOLUTION_DECODER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(SOLUTION_DECODER_NEW_BIN): $(SOLUTION_DECODER_NEW_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(SAT_SOLVER_NEW_BIN): $(SAT_SOLVER_NEW_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

visualize:
	@if [ -z "$(FILE)" ]; then echo "Usage: make visualize FILE=<basename>"; exit 1; fi
	@if [ ! -f "$(FILE).city" ]; then echo "File $(FILE).city not found"; exit 1; fi
	@if [ ! -f "$(FILE).metromap" ]; then echo "File $(FILE).metromap not found"; exit 1; fi
	@if [ ! -f "metro_visualizer.py" ]; then echo "metro_visualizer.py not found"; exit 1; fi
	python3 metro_visualizer.py "$(FILE).city" "$(FILE).metromap"

.PHONY: visualize

# Run SAT solver (GPT), minisat, and solution decoder on a sample basename
run_gpt: $(SAT_SOLVER_GPT_BIN) $(SOLUTION_DECODER_GPT_BIN)
	@if [ -z "$(FILE)" ]; then echo "Usage: make run_gpt FILE=<basename>"; exit 1; fi
	@./sat_solver_gpt sample/$(FILE).city && (minisat sample/$(FILE).satinput sample/$(FILE).satoutput || true) && ./solution_decoder_gpt sample/$(FILE)
	@python3 metro_visualizer.py sample/$(FILE).city sample/$(FILE).metromap
.PHONY: run_gpt


run_modified: $(SAT_SOLVER_NEW_BIN) $(SOLUTION_DECODER_NEW_BIN)
	@if [ -z "$(FILE)" ]; then echo "Usage: make run_gpt FILE=<basename>"; exit 1; fi
	@./sat_solver_new sample/$(FILE).city && (minisat sample/$(FILE).satinput sample/$(FILE).satoutput || true) && ./solution_decoder_new sample/$(FILE)
	@python3 metro_visualizer.py sample/$(FILE).city sample/$(FILE).metromap
.PHONY: run_gpt

# Clean built files
clean:
	rm -f $(SAT_SOLVER_BIN) $(SOLUTION_DECODER_BIN) $(SAT_SOLVER_GPT_BIN) $(SOLUTION_DECODER_GPT_BIN)

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