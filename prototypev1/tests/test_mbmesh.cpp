#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cassert>
#include <sstream>
#include <iomanip>

// Test framework macros
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running test: " << #name << "... "; \
    test_##name(); \
    std::cout << "PASSED" << std::endl; \
    tests_passed++; \
} while(0)

#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        std::cerr << "FAILED at line " << __LINE__ << ": " << #condition << std::endl; \
        exit(1); \
    } \
} while(0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        std::cerr << "FAILED at line " << __LINE__ << ": expected " << (expected) \
                  << " but got " << (actual) << std::endl; \
        exit(1); \
    } \
} while(0)

#define ASSERT_NEAR(expected, actual, epsilon) do { \
    if (std::abs((expected) - (actual)) > (epsilon)) { \
        std::cerr << "FAILED at line " << __LINE__ << ": expected " << (expected) \
                  << " but got " << (actual) << " (diff: " \
                  << std::abs((expected) - (actual)) << ")" << std::endl; \
        exit(1); \
    } \
} while(0)

// Include the structures from the main program
struct Point3D {
    double x, y, z;
    
    bool operator==(const Point3D& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct Triangle {
    int v0, v1, v2;
    
    bool operator==(const Triangle& other) const {
        return v0 == other.v0 && v1 == other.v1 && v2 == other.v2;
    }
};

class SwathData {
public:
    std::vector<Point3D> points;
    
    void generateSampleData(int width, int length) {
        points.clear();
        
        for (int i = 0; i < length; i++) {
            for (int j = 0; j < width; j++) {
                Point3D p;
                p.x = j * 10.0;
                p.y = i * 10.0;
                
                double base_depth = -100.0;
                double ridge = 20.0 * sin(j * 0.3) * cos(i * 0.2);
                double seamount = 30.0 * exp(-((j-width/2)*(j-width/2) + (i-length/2)*(i-length/2))/100.0);
                
                p.z = base_depth + ridge + seamount;
                
                points.push_back(p);
            }
        }
    }
};

class GLTFWriter {
private:
    std::vector<Point3D> vertices;
    std::vector<Triangle> triangles;
    
    void getBounds(Point3D& min, Point3D& max) {
        if (vertices.empty()) return;
        
        min = max = vertices[0];
        for (const auto& v : vertices) {
            min.x = std::min(min.x, v.x);
            min.y = std::min(min.y, v.y);
            min.z = std::min(min.z, v.z);
            max.x = std::max(max.x, v.x);
            max.y = std::max(max.y, v.y);
            max.z = std::max(max.z, v.z);
        }
    }
    
public:
    void createMeshFromSwath(const SwathData& swath, int width, int length) {
        vertices = swath.points;
        triangles.clear();
        
        for (int i = 0; i < length - 1; i++) {
            for (int j = 0; j < width - 1; j++) {
                int idx = i * width + j;
                
                Triangle t1 = {idx, idx + width, idx + 1};
                Triangle t2 = {idx + 1, idx + width, idx + width + 1};
                
                triangles.push_back(t1);
                triangles.push_back(t2);
            }
        }
    }
    
    // Getters for testing
    const std::vector<Point3D>& getVertices() const { return vertices; }
    const std::vector<Triangle>& getTriangles() const { return triangles; }
    
    bool writeGLTF(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        
        Point3D min, max;
        getBounds(min, max);
        
        file << "{\n";
        file << "  \"asset\": {\n";
        file << "    \"version\": \"2.0\",\n";
        file << "    \"generator\": \"mbmesh_prototype\"\n";
        file << "  },\n";
        file << "  \"scene\": 0,\n";
        file << "  \"scenes\": [{ \"nodes\": [0] }],\n";
        file << "  \"nodes\": [{ \"mesh\": 0 }],\n";
        file << "  \"meshes\": [{\n";
        file << "    \"primitives\": [{\n";
        file << "      \"attributes\": { \"POSITION\": 0 },\n";
        file << "      \"indices\": 1\n";
        file << "    }]\n";
        file << "  }],\n";
        file << "  \"accessors\": [\n";
        file << "    {\n";
        file << "      \"bufferView\": 0,\n";
        file << "      \"componentType\": 5126,\n";
        file << "      \"count\": " << vertices.size() << ",\n";
        file << "      \"type\": \"VEC3\",\n";
        file << "      \"max\": [" << max.x << "," << max.y << "," << max.z << "],\n";
        file << "      \"min\": [" << min.x << "," << min.y << "," << min.z << "]\n";
        file << "    },\n";
        file << "    {\n";
        file << "      \"bufferView\": 1,\n";
        file << "      \"componentType\": 5125,\n";
        file << "      \"count\": " << triangles.size() * 3 << ",\n";
        file << "      \"type\": \"SCALAR\"\n";
        file << "    }\n";
        file << "  ],\n";
        file << "  \"bufferViews\": [\n";
        file << "    { \"buffer\": 0, \"byteOffset\": 0, \"byteLength\": " << vertices.size() * 12 << " },\n";
        file << "    { \"buffer\": 0, \"byteOffset\": " << vertices.size() * 12 << ", \"byteLength\": " << triangles.size() * 12 << " }\n";
        file << "  ],\n";
        file << "  \"buffers\": [{\n";
        file << "    \"uri\": \"test_mesh.bin\",\n";
        file << "    \"byteLength\": " << (vertices.size() * 12 + triangles.size() * 12) << "\n";
        file << "  }]\n";
        file << "}\n";
        
        file.close();
        return true;
    }
    
    bool writeBinaryBuffer(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        for (const auto& v : vertices) {
            float x = static_cast<float>(v.x);
            float y = static_cast<float>(v.y);
            float z = static_cast<float>(v.z);
            file.write(reinterpret_cast<const char*>(&x), sizeof(float));
            file.write(reinterpret_cast<const char*>(&y), sizeof(float));
            file.write(reinterpret_cast<const char*>(&z), sizeof(float));
        }
        
        for (const auto& t : triangles) {
            unsigned int v0 = t.v0;
            unsigned int v1 = t.v1;
            unsigned int v2 = t.v2;
            file.write(reinterpret_cast<const char*>(&v0), sizeof(unsigned int));
            file.write(reinterpret_cast<const char*>(&v1), sizeof(unsigned int));
            file.write(reinterpret_cast<const char*>(&v2), sizeof(unsigned int));
        }
        
        file.close();
        return true;
    }
};

// Global test counter
int tests_passed = 0;

// ============================================================================
// UNIT TESTS
// ============================================================================

TEST(point3d_equality) {
    Point3D p1 = {1.0, 2.0, 3.0};
    Point3D p2 = {1.0, 2.0, 3.0};
    Point3D p3 = {1.0, 2.0, 4.0};
    
    ASSERT_TRUE(p1 == p2);
    ASSERT_FALSE(p1 == p3);
}

TEST(triangle_equality) {
    Triangle t1 = {0, 1, 2};
    Triangle t2 = {0, 1, 2};
    Triangle t3 = {0, 1, 3};
    
    ASSERT_TRUE(t1 == t2);
    ASSERT_FALSE(t1 == t3);
}

TEST(swath_data_generation_size) {
    SwathData swath;
    int width = 10;
    int length = 20;
    
    swath.generateSampleData(width, length);
    
    ASSERT_EQ(width * length, static_cast<int>(swath.points.size()));
}

TEST(swath_data_generation_coordinates) {
    SwathData swath;
    int width = 5;
    int length = 5;
    
    swath.generateSampleData(width, length);
    
    // Check first point (0, 0)
    ASSERT_NEAR(0.0, swath.points[0].x, 0.001);
    ASSERT_NEAR(0.0, swath.points[0].y, 0.001);
    
    // Check last point (4, 4)
    ASSERT_NEAR(40.0, swath.points[24].x, 0.001);  // (5-1) * 10
    ASSERT_NEAR(40.0, swath.points[24].y, 0.001);
    
    // Check depth is negative
    ASSERT_TRUE(swath.points[0].z < 0.0);
}

TEST(swath_data_grid_ordering) {
    SwathData swath;
    int width = 3;
    int length = 3;
    
    swath.generateSampleData(width, length);
    
    // Points should be ordered row by row
    // Row 0: (0,0), (10,0), (20,0)
    // Row 1: (0,10), (10,10), (20,10)
    // Row 2: (0,20), (10,20), (20,20)
    
    ASSERT_NEAR(0.0, swath.points[0].x, 0.001);
    ASSERT_NEAR(10.0, swath.points[1].x, 0.001);
    ASSERT_NEAR(20.0, swath.points[2].x, 0.001);
    ASSERT_NEAR(0.0, swath.points[3].x, 0.001);
    
    ASSERT_NEAR(0.0, swath.points[0].y, 0.001);
    ASSERT_NEAR(0.0, swath.points[1].y, 0.001);
    ASSERT_NEAR(10.0, swath.points[3].y, 0.001);
}

TEST(mesh_creation_vertex_count) {
    SwathData swath;
    int width = 10;
    int length = 20;
    
    swath.generateSampleData(width, length);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, width, length);
    
    ASSERT_EQ(width * length, static_cast<int>(gltf.getVertices().size()));
}

TEST(mesh_creation_triangle_count) {
    SwathData swath;
    int width = 10;
    int length = 20;
    
    swath.generateSampleData(width, length);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, width, length);
    
    // Each grid cell (width-1)*(length-1) creates 2 triangles
    int expected_triangles = (width - 1) * (length - 1) * 2;
    ASSERT_EQ(expected_triangles, static_cast<int>(gltf.getTriangles().size()));
}

TEST(mesh_creation_triangle_indices) {
    SwathData swath;
    int width = 3;
    int length = 3;
    
    swath.generateSampleData(width, length);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, width, length);
    
    // First grid cell should create triangles with vertices 0,3,1 and 1,3,4
    Triangle expected_t1 = {0, 3, 1};  // idx, idx+width, idx+1
    Triangle expected_t2 = {1, 3, 4};  // idx+1, idx+width, idx+width+1
    
    ASSERT_TRUE(gltf.getTriangles()[0] == expected_t1);
    ASSERT_TRUE(gltf.getTriangles()[1] == expected_t2);
}

TEST(mesh_all_indices_valid) {
    SwathData swath;
    int width = 10;
    int length = 20;
    
    swath.generateSampleData(width, length);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, width, length);
    
    int max_index = width * length - 1;
    
    for (const auto& tri : gltf.getTriangles()) {
        ASSERT_TRUE(tri.v0 >= 0 && tri.v0 <= max_index);
        ASSERT_TRUE(tri.v1 >= 0 && tri.v1 <= max_index);
        ASSERT_TRUE(tri.v2 >= 0 && tri.v2 <= max_index);
    }
}

TEST(gltf_file_creation) {
    SwathData swath;
    swath.generateSampleData(5, 5);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, 5, 5);
    
    bool success = gltf.writeGLTF("test_output.gltf");
    ASSERT_TRUE(success);
    
    // Check file exists
    std::ifstream file("test_output.gltf");
    ASSERT_TRUE(file.good());
    file.close();
    
    // Clean up
    std::remove("test_output.gltf");
}

TEST(binary_file_creation) {
    SwathData swath;
    swath.generateSampleData(5, 5);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, 5, 5);
    
    bool success = gltf.writeBinaryBuffer("test_output.bin");
    ASSERT_TRUE(success);
    
    // Check file exists
    std::ifstream file("test_output.bin", std::ios::binary);
    ASSERT_TRUE(file.good());
    
    // Check file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.close();
    
    // Expected size: (vertices * 12 bytes) + (triangles * 12 bytes)
    size_t expected_size = (25 * 12) + (32 * 12);  // 5*5 vertices, (4*4*2) triangles
    ASSERT_EQ(expected_size, file_size);
    
    // Clean up
    std::remove("test_output.bin");
}

TEST(gltf_json_valid_structure) {
    SwathData swath;
    swath.generateSampleData(5, 5);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, 5, 5);
    gltf.writeGLTF("test_structure.gltf");
    
    // Read file and check for required keys
    std::ifstream file("test_structure.gltf");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Check for required glTF fields
    ASSERT_TRUE(content.find("\"asset\"") != std::string::npos);
    ASSERT_TRUE(content.find("\"version\": \"2.0\"") != std::string::npos);
    ASSERT_TRUE(content.find("\"scenes\"") != std::string::npos);
    ASSERT_TRUE(content.find("\"nodes\"") != std::string::npos);
    ASSERT_TRUE(content.find("\"meshes\"") != std::string::npos);
    ASSERT_TRUE(content.find("\"accessors\"") != std::string::npos);
    ASSERT_TRUE(content.find("\"bufferViews\"") != std::string::npos);
    ASSERT_TRUE(content.find("\"buffers\"") != std::string::npos);
    
    // Clean up
    std::remove("test_structure.gltf");
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST(integration_small_mesh) {
    // Test complete pipeline with small mesh
    SwathData swath;
    swath.generateSampleData(3, 3);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, 3, 3);
    
    ASSERT_EQ(9, static_cast<int>(gltf.getVertices().size()));
    ASSERT_EQ(8, static_cast<int>(gltf.getTriangles().size()));  // (3-1)*(3-1)*2
    
    bool success = gltf.writeGLTF("test_small.gltf");
    ASSERT_TRUE(success);
    
    success = gltf.writeBinaryBuffer("test_small.bin");
    ASSERT_TRUE(success);
    
    // Clean up
    std::remove("test_small.gltf");
    std::remove("test_small.bin");
}

TEST(integration_medium_mesh) {
    // Test with realistic mesh size
    SwathData swath;
    swath.generateSampleData(50, 100);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, 50, 100);
    
    ASSERT_EQ(5000, static_cast<int>(gltf.getVertices().size()));
    ASSERT_EQ(9702, static_cast<int>(gltf.getTriangles().size()));  // (50-1)*(100-1)*2
    
    bool success = gltf.writeGLTF("test_medium.gltf");
    ASSERT_TRUE(success);
    
    success = gltf.writeBinaryBuffer("test_medium.bin");
    ASSERT_TRUE(success);
    
    // Clean up
    std::remove("test_medium.gltf");
    std::remove("test_medium.bin");
}

// ============================================================================
// VALIDATION TESTS
// ============================================================================

TEST(validation_no_degenerate_triangles) {
    // Ensure no triangle has duplicate vertices
    SwathData swath;
    swath.generateSampleData(10, 10);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, 10, 10);
    
    for (const auto& tri : gltf.getTriangles()) {
        ASSERT_TRUE(tri.v0 != tri.v1);
        ASSERT_TRUE(tri.v1 != tri.v2);
        ASSERT_TRUE(tri.v2 != tri.v0);
    }
}

TEST(validation_depth_values_negative) {
    // Bathymetry depths should be negative (below sea level)
    SwathData swath;
    swath.generateSampleData(10, 10);
    
    for (const auto& point : swath.points) {
        ASSERT_TRUE(point.z < 0.0);
    }
}

TEST(validation_xy_coordinates_positive) {
    // X and Y coordinates should be positive
    SwathData swath;
    swath.generateSampleData(10, 10);
    
    for (const auto& point : swath.points) {
        ASSERT_TRUE(point.x >= 0.0);
        ASSERT_TRUE(point.y >= 0.0);
    }
}

TEST(validation_consistent_spacing) {
    // Check that point spacing is consistent (10m)
    SwathData swath;
    swath.generateSampleData(5, 5);
    
    double expected_spacing = 10.0;
    
    // Check horizontal spacing
    for (int i = 0; i < 4; i++) {
        double spacing = swath.points[i+1].x - swath.points[i].x;
        ASSERT_NEAR(expected_spacing, spacing, 0.001);
    }
    
    // Check vertical spacing
    for (int i = 0; i < 20; i += 5) {
        double spacing = swath.points[i+5].y - swath.points[i].y;
        ASSERT_NEAR(expected_spacing, spacing, 0.001);
    }
}

TEST(validation_binary_file_size) {
    // Verify binary file has correct size
    SwathData swath;
    int width = 10;
    int length = 20;
    swath.generateSampleData(width, length);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, width, length);
    gltf.writeBinaryBuffer("test_size.bin");
    
    std::ifstream file("test_size.bin", std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t actual_size = file.tellg();
    file.close();
    
    size_t expected_size = (width * length * 12) + ((width-1)*(length-1)*2 * 12);
    ASSERT_EQ(expected_size, actual_size);
    
    // Clean up
    std::remove("test_size.bin");
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

TEST(edge_case_minimum_mesh) {
    // Test smallest valid mesh (2x2 = 4 vertices, 2 triangles)
    SwathData swath;
    swath.generateSampleData(2, 2);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, 2, 2);
    
    ASSERT_EQ(4, static_cast<int>(gltf.getVertices().size()));
    ASSERT_EQ(2, static_cast<int>(gltf.getTriangles().size()));
}

TEST(edge_case_single_row) {
    // Test mesh with single row
    SwathData swath;
    swath.generateSampleData(10, 2);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, 10, 2);
    
    ASSERT_EQ(20, static_cast<int>(gltf.getVertices().size()));
    ASSERT_EQ(18, static_cast<int>(gltf.getTriangles().size()));  // (10-1)*1*2
}

TEST(edge_case_single_column) {
    // Test mesh with single column
    SwathData swath;
    swath.generateSampleData(2, 10);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, 2, 10);
    
    ASSERT_EQ(20, static_cast<int>(gltf.getVertices().size()));
    ASSERT_EQ(18, static_cast<int>(gltf.getTriangles().size()));  // 1*(10-1)*2
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

TEST(performance_large_mesh) {
    // Test with large mesh to ensure no crashes
    SwathData swath;
    swath.generateSampleData(100, 200);
    
    GLTFWriter gltf;
    gltf.createMeshFromSwath(swath, 100, 200);
    
    ASSERT_EQ(20000, static_cast<int>(gltf.getVertices().size()));
    ASSERT_EQ(39402, static_cast<int>(gltf.getTriangles().size()));
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  MB-System mbmesh Test Suite\n";
    std::cout << "========================================\n";
    std::cout << "\n";
    
    std::cout << "--- Unit Tests ---\n";
    RUN_TEST(point3d_equality);
    RUN_TEST(triangle_equality);
    RUN_TEST(swath_data_generation_size);
    RUN_TEST(swath_data_generation_coordinates);
    RUN_TEST(swath_data_grid_ordering);
    RUN_TEST(mesh_creation_vertex_count);
    RUN_TEST(mesh_creation_triangle_count);
    RUN_TEST(mesh_creation_triangle_indices);
    RUN_TEST(mesh_all_indices_valid);
    RUN_TEST(gltf_file_creation);
    RUN_TEST(binary_file_creation);
    RUN_TEST(gltf_json_valid_structure);
    
    std::cout << "\n--- Integration Tests ---\n";
    RUN_TEST(integration_small_mesh);
    RUN_TEST(integration_medium_mesh);
    
    std::cout << "\n--- Validation Tests ---\n";
    RUN_TEST(validation_no_degenerate_triangles);
    RUN_TEST(validation_depth_values_negative);
    RUN_TEST(validation_xy_coordinates_positive);
    RUN_TEST(validation_consistent_spacing);
    RUN_TEST(validation_binary_file_size);
    
    std::cout << "\n--- Edge Case Tests ---\n";
    RUN_TEST(edge_case_minimum_mesh);
    RUN_TEST(edge_case_single_row);
    RUN_TEST(edge_case_single_column);
    
    std::cout << "\n--- Performance Tests ---\n";
    RUN_TEST(performance_large_mesh);
    
    std::cout << "\n========================================\n";
    std::cout << "  All " << tests_passed << " tests PASSED!\n";
    std::cout << "========================================\n";
    std::cout << "\n";
    
    return 0;
}
