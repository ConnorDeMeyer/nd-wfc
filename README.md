# N-Dimensional Wave Function Collapse (WFC) Library

A templated C++20 Wave Function Collapse engine that can work with 2D grids, 3D grids, Sudoku, and any other constraint satisfaction problem. The engine is coordinate-system agnostic and can work with any graph-like structure.

## Features

- **Templated Design**: Works with any World type that satisfies the `WorldConcept`
- **Builder Pattern**: Easy to use fluent interface for setting up WFC systems
- **Constraint System**: Flexible constraint definition using lambda functions
- **Multiple World Types**: Built-in support for 2D/3D arrays, graphs, and Sudoku grids
- **Coordinate Agnostic**: Works without coordinate systems using only cell IDs
- **C++20**: Uses modern C++ features like concepts, ranges, and smart pointers
- **Custom Random Selectors**: Pluggable random selection strategies for cell collapse, including compile-time compatible options

## Custom Random Selectors

The WFC library now supports customizable random selection strategies for choosing cell values during the collapse process. This allows users to implement different randomization algorithms while maintaining compile-time compatibility.

### Features

- **Default Random Selector**: A constexpr-compatible seed-based randomizer using linear congruential generator
- **Advanced Random Selector**: High-quality randomization using `std::mt19937` and `std::uniform_int_distribution`
- **Custom Selectors**: Support for user-defined selector classes that can capture state and maintain behavior between calls
- **Lambda Support**: Selectors can be implemented as lambdas with captured variables for flexible behavior

### Usage

#### Basic Usage with Default Selector

```cpp
using MyWFC = WFC::Builder<MyWorld, int, VariableMap>
    ::SetRandomSelector<WFC::DefaultRandomSelector<int>>
    ::Build;

MyWorld world(size);
MyWFC::Run(world, seed);
```

#### Advanced Random Selector

```cpp
using MyWFC = WFC::Builder<MyWorld, int, VariableMap>
    ::SetRandomSelector<WFC::AdvancedRandomSelector<int>>
    ::Build;

MyWorld world(size);
MyWFC::Run(world, seed);
```

#### Custom Selector Implementation

```cpp
class CustomSelector {
private:
    mutable int callCount = 0;

public:
    size_t operator()(std::span<const int> possibleValues) const {
        // Round-robin selection
        return callCount++ % possibleValues.size();
    }
};

using MyWFC = WFC::Builder<MyWorld, int, VariableMap>
    ::SetRandomSelector<CustomSelector>
    ::Build;
```

#### Stateful Lambda Selector

```cpp
int counter = 0;
auto selector = [&counter](std::span<const int> values) mutable -> size_t {
    return counter++ % values.size();
};

// Use with WFC (would need to wrap in a class for template parameter)
```

### Key Benefits

1. **Compile-time Compatible**: The default selector works in constexpr contexts for compile-time WFC solving
2. **Stateful Selection**: Selectors can maintain state between calls for deterministic or custom behaviors
3. **Flexible Interface**: Simple function signature `(std::span<const VarT>) -> size_t` makes implementation easy
4. **Performance**: Custom selectors can optimize for specific use cases (e.g., always pick first, weighted selection)

## Building the Project

### Prerequisites

- CMake 3.16 or higher
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- Git

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd nd-wfc

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
make

# Run basic test
./src/nd-wfc-main

# Run examples
./examples/tilemap_example
./examples/simple_sudoku_example
```

## Usage

### Basic Example (2x2 Grid)

```cpp
#include <nd-wfc/wfc.h>
#include <iostream>

int main() {
    enum SimpleTiles { A = 1, B = 2 };

    auto wfc = WFC::Builder<WFC::Array2D<uint8_t, 2, 2>>()
        .variable(A, [](WFC::Array2D<uint8_t, 2, 2>& world, int worldID,
                       WFC::Constrainer& constrainer, uint8_t var) {
            auto [x, y] = world.getCoord(worldID);
            if (y > 0) constrainer.only((y-1) * 2 + x, B);
            if (y < 1) constrainer.only((y+1) * 2 + x, B);
            if (x > 0) constrainer.only(y * 2 + (x-1), B);
            if (x < 1) constrainer.only(y * 2 + (x+1), B);
        })
        .variable(B, [](WFC::Array2D<uint8_t, 2, 2>& world, int worldID,
                       WFC::Constrainer& constrainer, uint8_t var) {
            auto [x, y] = world.getCoord(worldID);
            if (y > 0) constrainer.only((y-1) * 2 + x, A);
            if (y < 1) constrainer.only((y+1) * 2 + x, A);
            if (x > 0) constrainer.only(y * 2 + (x-1), A);
            if (x < 1) constrainer.only(y * 2 + (x+1), A);
        })
        .build(WFC::Array2D<uint8_t, 2, 2>());

    if (wfc->run()) {
        std::cout << "Solution found!" << std::endl;
        // Print solution...
    }

    return 0;
}
```

### 2D Tilemap Example

```cpp
enum TileType { SEA = 1, BEACH = 2, LAND = 3 };

auto wfc = WFC::Builder<WFC::Array2D<uint8_t, 100, 100>>()
    .variable(SEA, [](WFC::Array2D<uint8_t, 100, 100>& world, int worldID,
                    WFC::Constrainer& constrainer, uint8_t var) {
        auto [x, y] = world.getCoord(worldID);
        // Adjacent cells should be SEA or BEACH (no direct LAND next to SEA)
        if (y > 0) constrainer.only((y-1) * 100 + x, SEA, BEACH);
        if (y < 99) constrainer.only((y+1) * 100 + x, SEA, BEACH);
        if (x > 0) constrainer.only(y * 100 + (x-1), SEA, BEACH);
        if (x < 99) constrainer.only(y * 100 + (x+1), SEA, BEACH);
    })
    .variable(BEACH, [](WFC::Array2D<uint8_t, 100, 100>& world, int worldID,
                       WFC::Constrainer& constrainer, uint8_t var) {
        auto [x, y] = world.getCoord(worldID);
        // Adjacent cells should be SEA or LAND (BEACH is transition between them)
        if (y > 0) constrainer.only((y-1) * 100 + x, SEA, LAND);
        if (y < 99) constrainer.only((y+1) * 100 + x, SEA, LAND);
        if (x > 0) constrainer.only(y * 100 + (x-1), SEA, LAND);
        if (x < 99) constrainer.only(y * 100 + (x+1), SEA, LAND);
    })
    .variable(LAND, [](WFC::Array2D<uint8_t, 100, 100>& world, int worldID,
                     WFC::Constrainer& constrainer, uint8_t var) {
        auto [x, y] = world.getCoord(worldID);
        // Adjacent cells should be LAND or BEACH (no direct SEA next to LAND)
        if (y > 0) constrainer.only((y-1) * 100 + x, LAND, BEACH);
        if (y < 99) constrainer.only((y+1) * 100 + x, LAND, BEACH);
        if (x > 0) constrainer.only(y * 100 + (x-1), LAND, BEACH);
        if (x < 99) constrainer.only(y * 100 + (x+1), LAND, BEACH);
    })
    .build(WFC::Array2D<uint8_t, 100, 100>());
```

## World Types

The library provides several built-in World types:

### Array2D<T, Width, Height>
2D array with compile-time dimensions.

```cpp
WFC::Array2D<uint8_t, 100, 100> world;
```

### Array3D<T, Width, Height, Depth>
3D array with compile-time dimensions.

```cpp
WFC::Array3D<uint8_t, 10, 10, 10> world;
```

### GraphWorld<T>
Graph-based world for non-grid structures.

```cpp
WFC::GraphWorld<uint8_t> world(100); // 100 nodes
world.addEdge(0, 1); // Connect nodes
```

### SudokuWorld
Specialized 9x9 grid with Sudoku helper functions.

```cpp
WFC::SudokuWorld world;
auto rowCells = world.getRowCells(cellId);
auto colCells = world.getColumnCells(cellId);
auto boxCells = world.getBoxCells(cellId);
```

## Custom World Types

To create a custom World type, implement these requirements:

```cpp
struct MyWorld {
    using ValueType = MyValueType;
    using CoordType = MyCoordType;

    size_t size() const;
    int getId(CoordType coord) const;
    CoordType getCoord(int id) const;
};
```

## API Reference

### Builder<WorldT>

- `Builder& variable(VarT value, ConstraintFunc func)` - Add a variable with constraint function
- `std::unique_ptr<WFC<WorldT>> build(WorldT world)` - Build the WFC instance

### WFC<WorldT>

- `bool run()` - Run the WFC algorithm
- `std::optional<VarT> getValue(int cellId)` - Get value at cell (if collapsed)
- `const std::unordered_set<int>& getPossibleValues(int cellId)` - Get possible values for cell

### Constrainer

- `void constrain(int cellId, const std::unordered_set<int>& allowedValues)` - Constrain to specific values
- `void only(int cellId, int value)` - Constrain to single value
- `void only(int cellId, int value1, int value2)` - Constrain to two values

## Examples

The `examples/` directory contains:

- `tilemap_example.cpp` - 2D tilemap generation with Sea/Beach/Land
- `sudoku_example.cpp` - Full 9x9 Sudoku solver
- `simple_sudoku_example.cpp` - Simple 2x2 Sudoku for testing

## Running Examples

```bash
cd build/examples
./tilemap_example      # Generate a 100x100 tilemap
./simple_sudoku_example # Solve a simple 2x2 Sudoku
```

## Architecture

The WFC engine consists of:

1. **World Types**: Define the problem space and coordinate systems
2. **Builder Pattern**: Fluent interface for setting up variables and constraints
3. **Constraint System**: Lambda-based constraint definitions
4. **Wave Function Collapse Algorithm**: Core WFC implementation with observation and propagation
5. **Propagation Queue**: Efficient constraint propagation system

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## References

- Original WFC Algorithm: https://github.com/mxgmn/WaveFunctionCollapse
- Constraint satisfaction and procedural generation concepts
