CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2

TARGET = mbmesh_prototype
SOURCE = mbmesh_prototype.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) seafloor_mesh.gltf seafloor_mesh.bin

help:
	@echo "MB-System mbmesh Prototype Build System"
	@echo "========================================"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Compile the prototype"
	@echo "  make run      - Compile and run the prototype"
	@echo "  make clean    - Remove compiled files and generated meshes"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "To view the results:"
	@echo "  1. Run 'make run' to generate the mesh files"
	@echo "  2. Open viewer.html in a web browser"
	@echo "  3. (You may need to run a local web server)"

.PHONY: all run clean help
