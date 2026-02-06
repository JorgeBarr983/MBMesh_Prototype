CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2

TARGET = mbmesh_prototype
SOURCE = mbmesh_prototype.cpp
TEST_TARGET = test_mbmesh
TEST_SOURCE = test_mbmesh.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

$(TEST_TARGET): $(TEST_SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SOURCE)

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGET)
	@echo "Running C++ unit tests..."
	./$(TEST_TARGET)

test-validation: $(TARGET)
	@echo "Running Python validation tests..."
	./test_validation.py

test-all: test test-validation
	@echo ""
	@echo "All test suites completed!"

clean:
	rm -f $(TARGET) $(TEST_TARGET) seafloor_mesh.gltf seafloor_mesh.bin
	rm -f test_*.gltf test_*.bin bench_temp bench_temp.cpp

help:
	@echo "MB-System mbmesh Prototype Build System"
	@echo "========================================"
	@echo ""
	@echo "Available targets:"
	@echo "  make               - Compile the prototype"
	@echo "  make run           - Compile and run the prototype"
	@echo "  make test          - Compile and run C++ unit tests"
	@echo "  make test-validation - Run Python validation tests"
	@echo "  make test-all      - Run all test suites"
	@echo "  make clean         - Remove compiled files and generated meshes"
	@echo "  make help          - Show this help message"
	@echo ""
	@echo "To view the results:"
	@echo "  1. Run 'make run' to generate the mesh files"
	@echo "  2. Open viewer.html in a web browser"
	@echo "  3. (You may need to run a local web server)"

.PHONY: all run test test-validation test-all clean help
