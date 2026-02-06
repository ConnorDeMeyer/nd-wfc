# Fast Sudoku Loader and Solution Validator

A memory-efficient and fast Sudoku implementation with puzzle loading and solution validation capabilities.

## Features

- **Ultra-Memory Efficient**: Uses exactly 41 bytes per Sudoku instance
  - 4-bit packed storage (81 cells × 4 bits = 324 bits = 40.5 bytes → 41 bytes)
  - Static assertion ensures exactly 41 bytes
  - ~70% memory reduction compared to previous implementation
- **Ultra-Fast Operations**: Bitwise-only operations with strategic inlining and hot path optimizations
- **Multiple Input Formats**: Load from strings, files, or directories
- **Comprehensive Validation**: Check validity, completeness, and find conflicts
- **Easy to Use**: Simple API with comprehensive examples

## Memory Usage

```
Sudoku Class: Exactly 41 bytes total
└── 4-bit packed board: 41 bytes (81 cells × 4 bits = 324 bits)

Memory savings: 94 bytes per instance (69.6% reduction)
```

## Quick Start

### Building

```bash
cd /home/connor/repos/nd-wfc/demos/sudoku
mkdir build && cd build
cmake ..
make
./bin/sudoku_demo
```

### Basic Usage

```cpp
#include "sudoku.h"

// Create and load a puzzle from string
Sudoku sudoku("530070000600195000098000060800060003400803001700020006060000280000419005000080079");

// Check if it's valid
if (sudoku.isValid()) {
    std::cout << "Valid puzzle!" << std::endl;
}

// Make a move
if (sudoku.set(0, 1, 2)) {  // Set row 0, col 1 to 2
    std::cout << "Move successful!" << std::endl;
}

// Check if solved
if (sudoku.isSolved()) {
    std::cout << "Puzzle solved!" << std::endl;
}
```

### Loading Puzzles

```cpp
// From string
auto sudoku1 = SudokuLoader::fromString("530070000600195000098000060800060003400803001700020006060000280000419005000080079");

// From file
auto sudoku2 = SudokuLoader::fromFile("puzzle.txt");

// From directory (all .txt files)
auto puzzles = SudokuLoader::fromDirectory("./puzzles/", ".txt");
```

### Validating Solutions

```cpp
// Stateless validation using raw board data
Sudoku::Board board = sudoku.getBoard();

if (SudokuValidator::isValidSolution(board)) {
    std::cout << "Valid complete solution!" << std::endl;
}

if (SudokuValidator::hasConflicts(board)) {
    auto conflicts = SudokuValidator::findConflicts(board);
    std::cout << "Found " << conflicts.size() << " conflicts" << std::endl;
}
```

## Input Formats

### String Format
81 characters, using:
- `0` or `.` for empty cells
- `1-9` for filled cells

Example:
```
530070000600195000098000060800060003400803001700020006060000280000419005000080079
```

### File Format
Supports comments (#) and whitespace. Can be split across multiple lines.

Example:
```
# Easy Sudoku Puzzle
530070000
600195000
098000060
800060003
400803001
700020006
060000280
000419005
000080079
```

## API Reference

### Sudoku Class

#### Constructors
- `Sudoku()` - Create empty puzzle
- `Sudoku(const std::string& puzzle_str)` - Create from string

#### Loading
- `bool loadFromString(const std::string& puzzle_str)`
- `bool loadFromFile(const std::string& filename)`

#### Board Operations (Inlined for Performance)
- `inline uint8_t get(int row, int col)` - Get cell value with bounds assertion (ultra-fast)
- `inline bool set(int row, int col, uint8_t value)` - Set cell value with bounds assertion (ultra-fast)
- `void clear()` - Clear the board

#### Validation
- `bool isValid()` - Check if current board is valid
- `bool isSolved()` - Check if puzzle is complete and valid
- `inline bool isValidMove(int row, int col, uint8_t value)` - Check if move is valid (inlined for performance)

#### Utilities
- `void print()` - Print board to console
- `std::string toString()` - Convert to string
- `std::array<uint8_t, 81> getBoard()` - Convert to standard board format

### SudokuValidator Class (Static)

#### Validation Functions
- `bool isValidSolution(const std::array<uint8_t, 81>& board)` - Check complete solution
- `bool isValidPartial(const std::array<uint8_t, 81>& board)` - Check partial solution
- `bool hasConflicts(const std::array<uint8_t, 81>& board)` - Check for any conflicts
- `std::vector<std::pair<int, int>> findConflicts(const std::array<uint8_t, 81>& board)` - Find conflicting positions

### SudokuLoader Class (Static)

#### Loading Functions
- `std::optional<Sudoku> fromString(const std::string& puzzle_str)`
- `std::optional<Sudoku> fromFile(const std::string& filename)`
- `std::vector<Sudoku> fromDirectory(const std::string& dirname, const std::string& extension = ".txt")`

#### Storage Implementation
- `uint8_t get(int pos)` - Get value with range assertion (0-9)
- `void set(int pos, uint8_t value)` - Set value with range assertion (0-9)

#### Private Helper Functions
- `bool parseLine(const std::string& line, std::array<uint8_t, 81>& board)`

## Sample Puzzles

### Easy Puzzle
```
530070000600195000098000060800060003400803001700020006060000280000419005000080079
```

### Medium Puzzle
```
003020600900305001001806400008102900700000008006708200002609500800203009005010300
```

### Hard Puzzle
```
400000805030000000000700000020000060000080400000010000000603070500200000104000000
```

### Solved Puzzle
```
534678912672195348198342567859761423426853791713924856961537284287419635345286179
```

## Performance

The implementation is optimized for speed and memory usage:

- **Validation**: ~0.11 microseconds per check
- **Memory**: Exactly 41 bytes per instance
- **Move validation**: Ultra-fast inlined bitwise operations only
- **Conflict detection**: Inlined direct board scanning with `std::bitset<10>` optimization
- **Position bounds**: Assert-based checking (zero overhead in release)

Run the demo to see performance benchmarks:
```bash
./bin/sudoku_demo
```

## Testing and Benchmarking

The project includes comprehensive Google Test and Google Benchmark integration:

**Note**: Google Test and Google Benchmark are optional dependencies. If not found on your system, the project will still build and run the main demo, but tests and benchmarks will not be available.

### Installing Dependencies (Optional)

#### Ubuntu/Debian:
```bash
# Install Google Test
sudo apt-get install libgtest-dev

# Install Google Benchmark
sudo apt-get install libbenchmark-dev
```

#### macOS (with Homebrew):
```bash
# Install Google Test
brew install googletest

# Install Google Benchmark
brew install google-benchmark
```

#### Windows (with vcpkg):
```bash
# Install Google Test
vcpkg install gtest

# Install Google Benchmark
vcpkg install benchmark
```

### Running Tests
```bash
# Build and run all tests
./build.sh test

# Or run tests directly
./bin/sudoku_tests
```

### Running Benchmarks
```bash
# Build and run all benchmarks
./build.sh benchmark

# Or run benchmarks directly
./bin/sudoku_benchmarks
```

### Available Tests
- **Basic functionality**: Empty sudoku, set/get operations, loading
- **Validation**: Move validation, conflict detection, solved puzzles
- **Edge cases**: Invalid inputs, boundary conditions
- **Performance**: Timing tests for operations
- **Memory**: Size validation (41 bytes)

### Available Benchmarks
- **Core operations**: get/set performance
- **Validation**: isValid/isValidMove timing
- **Conversion**: toString/getBoard performance
- **Puzzle complexity**: Easy/medium/hard/solved puzzle benchmarks
- **Memory operations**: Allocation/deallocation timing
- **Validator functions**: Static validation performance

### Test Output Example
```
[==========] Running 18 tests from 1 test case.
[----------] Global test environment set-up.
[----------] 18 tests from SudokuTest (X ms total)

[ RUN      ] SudokuTest.MemorySize
[       OK ] SudokuTest.MemorySize (0 ms)
[ RUN      ] SudokuTest.SetAndGet
[       OK ] SudokuTest.SetAndGet (0 ms)
...
[==========] 18 tests from 1 test case ran. (X ms total)
[  PASSED  ] 18 tests.
```

## Building and Running

```bash
# Navigate to the sudoku demo directory
cd /home/connor/repos/nd-wfc/demos/sudoku

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Run demo
./bin/sudoku_demo
```

## Testing

The demo program includes comprehensive tests for:
- Basic functionality
- Memory efficiency
- Performance benchmarks
- Loading from various formats
- Validation accuracy

Run the demo to see all tests in action.
