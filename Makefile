# Makefile for Bitcoin Puzzle #73 Kangaroo Solver

CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -mtune=native -Wall -Wextra -fopenmp
LDFLAGS = -lgmp -lcrypto -ljsoncpp -fopenmp

# Source files
SOURCES = kangaroo_solver.cpp ecc_utils.cpp checkpoint.cpp
HEADERS = kangaroo_solver.h ecc_utils.h checkpoint.h
OBJECTS = $(SOURCES:.cpp=.o)

# Target library
TARGET = libkangaroo.so

# Default target
all: $(TARGET)

# Build shared library
$(TARGET): $(OBJECTS)
	$(CXX) -shared -fPIC $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Build object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f *.checkpoint *.dat *.log

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y \
		build-essential \
		libgmp-dev \
		libssl-dev \
		libjsoncpp-dev \
		python3-dev \
		python3-pip

# Install Python dependencies
install-python-deps:
	pip3 install --user -r requirements.txt

# Full setup
setup: install-deps install-python-deps all

# Debug build
debug: CXXFLAGS = -std=c++17 -g -O0 -Wall -Wextra -fopenmp -DDEBUG
debug: $(TARGET)

# Profile build
profile: CXXFLAGS = -std=c++17 -O3 -pg -march=native -mtune=native -Wall -Wextra -fopenmp
profile: $(TARGET)

# Test the library
test: $(TARGET)
	python3 -c "import ctypes; lib = ctypes.CDLL('./$(TARGET)'); print('Library loaded successfully')"

# Run the solver
run: $(TARGET)
	python3 main.py

# Create package
package: clean all
	mkdir -p kangaroo_solver_package
	cp $(TARGET) kangaroo_solver_package/
	cp *.py kangaroo_solver_package/
	cp *.html *.css *.js kangaroo_solver_package/
	cp Makefile build.sh kangaroo_solver_package/
	cp README.md kangaroo_solver_package/ 2>/dev/null || true
	tar -czf kangaroo_solver.tar.gz kangaroo_solver_package/
	rm -rf kangaroo_solver_package/

# Check for memory leaks (requires valgrind)
memcheck: debug
	valgrind --leak-check=full --show-leak-kinds=all python3 main.py

# Performance profiling (requires gprof)
prof: profile
	python3 main.py
	gprof $(TARGET) gmon.out > analysis.txt

# Static analysis (requires cppcheck)
analyze:
	cppcheck --enable=all --std=c++17 $(SOURCES)

# Format code (requires clang-format)
format:
	clang-format -i $(SOURCES) $(HEADERS)

# Show help
help:
	@echo "Available targets:"
	@echo "  all            - Build the library (default)"
	@echo "  clean          - Remove build files"
	@echo "  install-deps   - Install system dependencies"
	@echo "  install-python-deps - Install Python dependencies"
	@echo "  setup          - Full setup (dependencies + build)"
	@echo "  debug          - Build debug version"
	@echo "  profile        - Build profile version"
	@echo "  test           - Test library loading"
	@echo "  run            - Run the solver"
	@echo "  package        - Create distribution package"
	@echo "  memcheck       - Check for memory leaks"
	@echo "  prof           - Performance profiling"
	@echo "  analyze        - Static code analysis"
	@echo "  format         - Format source code"
	@echo "  help           - Show this help"

.PHONY: all clean install-deps install-python-deps setup debug profile test run package memcheck prof analyze format help
