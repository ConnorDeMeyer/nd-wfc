#!/bin/bash

# Simple build script for the Sudoku demo

set -e  # Exit on any error

echo "Building Fast Sudoku Loader and Solution Validator..."
echo "======================================================"

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
echo "Building project..."
make -j$(nproc)

echo ""
echo "Build completed successfully!"
echo "Run the demo with: ./bin/sudoku_demo"

# Check what executables were built
available_executables=""
if [ -f "./bin/sudoku_tests" ]; then
    echo "Run tests with: ./bin/sudoku_tests"
    available_executables="$available_executables test"
fi

if [ -f "./bin/sudoku_benchmarks" ]; then
    echo "Run benchmarks with: ./bin/sudoku_benchmarks"
    available_executables="$available_executables benchmark"
fi

echo ""

# Handle different run modes
case "$1" in
    "run")
        echo "Running demo..."
        ./bin/sudoku_demo
        ;;
    "test")
        if [ -f "./bin/sudoku_tests" ]; then
            echo "Running tests..."
            ./bin/sudoku_tests
        else
            echo "Tests not available (Google Test not found)"
        fi
        ;;
    "benchmark")
        if [ -f "./bin/sudoku_benchmarks" ]; then
            echo "Running benchmarks..."
            ./bin/sudoku_benchmarks
        else
            echo "Benchmarks not available (Google Benchmark not found)"
        fi
        ;;
    "all")
        echo "Running demo..."
        ./bin/sudoku_demo
        echo ""

        if [ -f "./bin/sudoku_tests" ]; then
            echo "Running tests..."
            ./bin/sudoku_tests
            echo ""
        else
            echo "Tests not available (Google Test not found)"
            echo ""
        fi

        if [ -f "./bin/sudoku_benchmarks" ]; then
            echo "Running benchmarks..."
            ./bin/sudoku_benchmarks
        else
            echo "Benchmarks not available (Google Benchmark not found)"
        fi
        ;;
    *)
        if [ -n "$1" ]; then
            echo "Unknown option: $1"
            echo "Usage: $0 [run|test|benchmark|all]"
        fi
        ;;
esac
