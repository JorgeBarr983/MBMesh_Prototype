#!/usr/bin/env python3
"""
Advanced validation and benchmarking tests for mbmesh prototype
Tests glTF file format compliance, data integrity, and performance
"""

import json
import struct
import os
import sys
import time
import subprocess
from pathlib import Path


class Colors:
    """ANSI color codes for terminal output"""
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'


def print_header(text):
    print(f"\n{Colors.HEADER}{Colors.BOLD}=== {text} ==={Colors.ENDC}")


def print_test(name):
    print(f"{Colors.OKCYAN}Testing: {name}...{Colors.ENDC}", end=" ")


def print_pass():
    print(f"{Colors.OKGREEN}PASSED{Colors.ENDC}")


def print_fail(msg=""):
    print(f"{Colors.FAIL}FAILED{Colors.ENDC}")
    if msg:
        print(f"  {Colors.FAIL}Error: {msg}{Colors.ENDC}")


def test_gltf_json_format(gltf_path):
    """Test that glTF JSON is valid and well-formed"""
    print_test("glTF JSON format validation")
    
    try:
        with open(gltf_path, 'r') as f:
            data = json.load(f)
        
        # Check required top-level fields
        required_fields = ['asset', 'scenes', 'nodes', 'meshes', 
                          'accessors', 'bufferViews', 'buffers']
        
        for field in required_fields:
            if field not in data:
                print_fail(f"Missing required field: {field}")
                return False
        
        # Check asset version
        if data['asset']['version'] != '2.0':
            print_fail(f"Invalid glTF version: {data['asset']['version']}")
            return False
        
        print_pass()
        return True
    
    except json.JSONDecodeError as e:
        print_fail(f"Invalid JSON: {e}")
        return False
    except Exception as e:
        print_fail(str(e))
        return False


def test_gltf_accessor_bounds(gltf_path, bin_path):
    """Test that accessor min/max bounds are correct"""
    print_test("Accessor bounds validation")
    
    try:
        with open(gltf_path, 'r') as f:
            gltf = json.load(f)
        
        # Get position accessor
        position_accessor = gltf['accessors'][0]
        
        # Read binary data
        with open(bin_path, 'rb') as f:
            vertex_data = f.read(position_accessor['count'] * 12)  # 3 floats * 4 bytes
        
        # Parse vertices
        vertices = []
        for i in range(position_accessor['count']):
            offset = i * 12
            x, y, z = struct.unpack('fff', vertex_data[offset:offset+12])
            vertices.append((x, y, z))
        
        # Calculate actual bounds
        xs = [v[0] for v in vertices]
        ys = [v[1] for v in vertices]
        zs = [v[2] for v in vertices]
        
        actual_min = [min(xs), min(ys), min(zs)]
        actual_max = [max(xs), max(ys), max(zs)]
        
        # Compare with accessor bounds
        declared_min = position_accessor['min']
        declared_max = position_accessor['max']
        
        epsilon = 0.001
        for i in range(3):
            if abs(actual_min[i] - declared_min[i]) > epsilon:
                print_fail(f"Min bound mismatch at component {i}")
                return False
            if abs(actual_max[i] - declared_max[i]) > epsilon:
                print_fail(f"Max bound mismatch at component {i}")
                return False
        
        print_pass()
        return True
    
    except Exception as e:
        print_fail(str(e))
        return False


def test_triangle_indices_valid(gltf_path, bin_path):
    """Test that all triangle indices are valid"""
    print_test("Triangle index validation")
    
    try:
        with open(gltf_path, 'r') as f:
            gltf = json.load(f)
        
        vertex_count = gltf['accessors'][0]['count']
        index_accessor = gltf['accessors'][1]
        index_count = index_accessor['count']
        
        # Read index data
        with open(bin_path, 'rb') as f:
            f.seek(gltf['bufferViews'][1]['byteOffset'])
            index_data = f.read(index_count * 4)  # 4 bytes per uint32
        
        # Check each index
        for i in range(index_count):
            offset = i * 4
            index = struct.unpack('I', index_data[offset:offset+4])[0]
            
            if index >= vertex_count:
                print_fail(f"Index {index} out of range (max: {vertex_count-1})")
                return False
        
        print_pass()
        return True
    
    except Exception as e:
        print_fail(str(e))
        return False


def test_no_degenerate_triangles(gltf_path, bin_path):
    """Test that no triangles have duplicate vertices"""
    print_test("Degenerate triangle check")
    
    try:
        with open(gltf_path, 'r') as f:
            gltf = json.load(f)
        
        index_count = gltf['accessors'][1]['count']
        
        # Read index data
        with open(bin_path, 'rb') as f:
            f.seek(gltf['bufferViews'][1]['byteOffset'])
            index_data = f.read(index_count * 4)
        
        # Check triangles (every 3 indices)
        for i in range(0, index_count, 3):
            i0 = struct.unpack('I', index_data[i*4:(i+1)*4])[0]
            i1 = struct.unpack('I', index_data[(i+1)*4:(i+2)*4])[0]
            i2 = struct.unpack('I', index_data[(i+2)*4:(i+3)*4])[0]
            
            if i0 == i1 or i1 == i2 or i2 == i0:
                print_fail(f"Degenerate triangle at index {i//3}")
                return False
        
        print_pass()
        return True
    
    except Exception as e:
        print_fail(str(e))
        return False


def test_buffer_size_matches(gltf_path, bin_path):
    """Test that binary buffer size matches declaration"""
    print_test("Buffer size consistency")
    
    try:
        with open(gltf_path, 'r') as f:
            gltf = json.load(f)
        
        declared_size = gltf['buffers'][0]['byteLength']
        actual_size = os.path.getsize(bin_path)
        
        if declared_size != actual_size:
            print_fail(f"Size mismatch: declared={declared_size}, actual={actual_size}")
            return False
        
        print_pass()
        return True
    
    except Exception as e:
        print_fail(str(e))
        return False


def test_mesh_topology(gltf_path, bin_path):
    """Test mesh topology and connectivity"""
    print_test("Mesh topology analysis")
    
    try:
        with open(gltf_path, 'r') as f:
            gltf = json.load(f)
        
        vertex_count = gltf['accessors'][0]['count']
        triangle_count = gltf['accessors'][1]['count'] // 3
        
        # Read triangles
        with open(bin_path, 'rb') as f:
            f.seek(gltf['bufferViews'][1]['byteOffset'])
            index_data = f.read(gltf['accessors'][1]['count'] * 4)
        
        # Build vertex usage map
        vertex_usage = [0] * vertex_count
        
        for i in range(0, gltf['accessors'][1]['count'], 3):
            i0 = struct.unpack('I', index_data[i*4:(i+1)*4])[0]
            i1 = struct.unpack('I', index_data[(i+1)*4:(i+2)*4])[0]
            i2 = struct.unpack('I', index_data[(i+2)*4:(i+3)*4])[0]
            
            vertex_usage[i0] += 1
            vertex_usage[i1] += 1
            vertex_usage[i2] += 1
        
        # Check for unused vertices
        unused_vertices = sum(1 for u in vertex_usage if u == 0)
        
        if unused_vertices > 0:
            print_fail(f"{unused_vertices} unused vertices found")
            return False
        
        print_pass()
        print(f"  {Colors.OKBLUE}Vertices: {vertex_count}, Triangles: {triangle_count}{Colors.ENDC}")
        return True
    
    except Exception as e:
        print_fail(str(e))
        return False


def benchmark_generation():
    """Benchmark mesh generation performance"""
    print_header("Performance Benchmarks")
    
    test_sizes = [
        (10, 20, "Small"),
        (50, 100, "Medium"),
        (100, 200, "Large"),
    ]
    
    for width, length, label in test_sizes:
        print(f"{Colors.OKCYAN}Benchmarking {label} mesh ({width}x{length})...{Colors.ENDC}")
        
        # Create temporary test program
        test_code = f"""
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>

struct Point3D {{ double x, y, z; }};

std::vector<Point3D> generateData(int w, int l) {{
    std::vector<Point3D> points;
    for (int i = 0; i < l; i++) {{
        for (int j = 0; j < w; j++) {{
            Point3D p;
            p.x = j * 10.0;
            p.y = i * 10.0;
            p.z = -100.0 + 20.0 * sin(j * 0.3) * cos(i * 0.2);
            points.push_back(p);
        }}
    }}
    return points;
}}

int main() {{
    auto start = std::chrono::high_resolution_clock::now();
    auto data = generateData({width}, {length});
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << duration.count() << std::endl;
    return 0;
}}
"""
        
        with open('bench_temp.cpp', 'w') as f:
            f.write(test_code)
        
        # Compile and run
        try:
            subprocess.run(['g++', '-std=c++11', '-O2', '-o', 'bench_temp', 'bench_temp.cpp'],
                          check=True, capture_output=True)
            result = subprocess.run(['./bench_temp'], capture_output=True, text=True, check=True)
            
            microseconds = int(result.stdout.strip())
            milliseconds = microseconds / 1000.0
            
            vertex_count = width * length
            triangle_count = (width - 1) * (length - 1) * 2
            
            print(f"  Time: {milliseconds:.2f}ms")
            print(f"  Vertices: {vertex_count:,}, Triangles: {triangle_count:,}")
            print(f"  Rate: {vertex_count/milliseconds:.0f} vertices/ms")
            
            # Cleanup
            os.remove('bench_temp.cpp')
            os.remove('bench_temp')
            
        except subprocess.CalledProcessError as e:
            print(f"  {Colors.WARNING}Benchmark failed{Colors.ENDC}")


def test_data_integrity():
    """Test data integrity and consistency"""
    print_test("Data integrity check")
    
    try:
        # Generate a test mesh
        result = subprocess.run(['./mbmesh_prototype'], 
                              capture_output=True, text=True, timeout=10)
        
        if result.returncode != 0:
            print_fail("Prototype execution failed")
            return False
        
        # Check output files exist
        if not os.path.exists('seafloor_mesh.gltf'):
            print_fail("glTF file not created")
            return False
        
        if not os.path.exists('seafloor_mesh.bin'):
            print_fail("Binary file not created")
            return False
        
        print_pass()
        return True
    
    except Exception as e:
        print_fail(str(e))
        return False


def main():
    print(f"{Colors.BOLD}")
    print("=" * 60)
    print("  MB-System mbmesh - Python Validation & Benchmark Suite")
    print("=" * 60)
    print(f"{Colors.ENDC}")
    
    # Check if prototype exists
    if not os.path.exists('./mbmesh_prototype'):
        print(f"{Colors.FAIL}Error: mbmesh_prototype not found. Please compile first.{Colors.ENDC}")
        print("Run: make")
        return 1
    
    # Run prototype to generate test files
    print_header("Generating Test Data")
    if not test_data_integrity():
        return 1
    
    gltf_file = 'seafloor_mesh.gltf'
    bin_file = 'seafloor_mesh.bin'
    
    # Run validation tests
    print_header("Format Validation Tests")
    
    tests = [
        (test_gltf_json_format, [gltf_file]),
        (test_buffer_size_matches, [gltf_file, bin_file]),
        (test_gltf_accessor_bounds, [gltf_file, bin_file]),
        (test_triangle_indices_valid, [gltf_file, bin_file]),
        (test_no_degenerate_triangles, [gltf_file, bin_file]),
        (test_mesh_topology, [gltf_file, bin_file]),
    ]
    
    passed = 0
    failed = 0
    
    for test_func, args in tests:
        if test_func(*args):
            passed += 1
        else:
            failed += 1
    
    # Run benchmarks
    benchmark_generation()
    
    # Summary
    print_header("Test Summary")
    print(f"Passed: {Colors.OKGREEN}{passed}{Colors.ENDC}")
    print(f"Failed: {Colors.FAIL if failed > 0 else Colors.OKGREEN}{failed}{Colors.ENDC}")
    
    if failed == 0:
        print(f"\n{Colors.OKGREEN}{Colors.BOLD}All validation tests PASSED!{Colors.ENDC}\n")
        return 0
    else:
        print(f"\n{Colors.FAIL}{Colors.BOLD}Some tests FAILED!{Colors.ENDC}\n")
        return 1


if __name__ == '__main__':
    sys.exit(main())
