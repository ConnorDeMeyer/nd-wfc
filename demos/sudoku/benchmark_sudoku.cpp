#include <benchmark/benchmark.h>
#include "sudoku.h"

// Benchmark fixture for Sudoku benchmarks
class SudokuBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        // Create test puzzle
        testPuzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
        sudoku.loadFromString(testPuzzle);
    }

    void TearDown(const ::benchmark::State&) override {
        // Cleanup if needed
    }

    Sudoku sudoku;
    std::string testPuzzle;
};

// Benchmark get operations
BENCHMARK_F(SudokuBenchmark, GetOperations)(benchmark::State& state) {
    for (auto _ : state) {
        // Perform get operations on all cells
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                benchmark::DoNotOptimize(sudoku.get(row, col));
            }
        }
    }
}

// Benchmark set operations
BENCHMARK_F(SudokuBenchmark, SetOperations)(benchmark::State& state) {
    for (auto _ : state) {
        // Clear and set all cells
        sudoku.clear();
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                sudoku.set(row, col, (row + col) % 9 + 1);
            }
        }
    }
}

// Benchmark validation operations
BENCHMARK_F(SudokuBenchmark, IsValidOperation)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(sudoku.isValid());
    }
}

// Benchmark isValidMove operations (includes conflict checking internally)
BENCHMARK_F(SudokuBenchmark, IsValidMoveOperation)(benchmark::State& state) {
    for (auto _ : state) {
        // Test various move validations
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                benchmark::DoNotOptimize(sudoku.isValidMove(row, col, (row + col) % 9 + 1));
            }
        }
    }
}

// Benchmark loading from string
BENCHMARK_F(SudokuBenchmark, LoadFromString)(benchmark::State& state) {
    for (auto _ : state) {
        Sudoku tempSudoku;
        tempSudoku.loadFromString(testPuzzle);
        benchmark::DoNotOptimize(tempSudoku);
    }
}

// Benchmark toString conversion
BENCHMARK_F(SudokuBenchmark, ToStringConversion)(benchmark::State& state) {
    for (auto _ : state) {
        std::string str = sudoku.toString();
        benchmark::DoNotOptimize(str);
    }
}

// Benchmark getBoard conversion
BENCHMARK_F(SudokuBenchmark, GetBoardConversion)(benchmark::State& state) {
    for (auto _ : state) {
        auto board = sudoku.getBoard();
        benchmark::DoNotOptimize(board);
    }
}

// Benchmark memory operations
BENCHMARK_F(SudokuBenchmark, MemoryOperations)(benchmark::State& state) {
    for (auto _ : state) {
        // Test memory allocation and deallocation
        Sudoku* temp = new Sudoku();
        temp->loadFromString(testPuzzle);
        delete temp;
    }
}



// Benchmark with different puzzle complexities
static void BM_EasyPuzzle(benchmark::State& state) {
    Sudoku sudoku;
    std::string easy = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
    sudoku.loadFromString(easy);

    for (auto _ : state) {
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                benchmark::DoNotOptimize(sudoku.get(row, col));
            }
        }
    }
}

static void BM_MediumPuzzle(benchmark::State& state) {
    Sudoku sudoku;
    std::string medium = "003020600900305001001806400008102900700000008006708200002609500800203009005010300";
    sudoku.loadFromString(medium);

    for (auto _ : state) {
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                benchmark::DoNotOptimize(sudoku.get(row, col));
            }
        }
    }
}

static void BM_HardPuzzle(benchmark::State& state) {
    Sudoku sudoku;
    std::string hard = "400000805030000000000700000020000060000080400000010000000603070500200000104000000";
    sudoku.loadFromString(hard);

    for (auto _ : state) {
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                benchmark::DoNotOptimize(sudoku.get(row, col));
            }
        }
    }
}

static void BM_SolvedPuzzle(benchmark::State& state) {
    Sudoku sudoku;
    std::string solved = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";
    sudoku.loadFromString(solved);

    for (auto _ : state) {
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                benchmark::DoNotOptimize(sudoku.get(row, col));
            }
        }
    }
}

// Register different puzzle complexity benchmarks
BENCHMARK(BM_EasyPuzzle)->Name("Easy_Puzzle_Get");
BENCHMARK(BM_MediumPuzzle)->Name("Medium_Puzzle_Get");
BENCHMARK(BM_HardPuzzle)->Name("Hard_Puzzle_Get");
BENCHMARK(BM_SolvedPuzzle)->Name("Solved_Puzzle_Get");

// Benchmark SudokuValidator functions
static void BM_ValidatorValidSolution(benchmark::State& state) {
    std::array<uint8_t, 81> solved = {
        5,3,4,6,7,8,9,1,2,
        6,7,2,1,9,5,3,4,8,
        1,9,8,3,4,2,5,6,7,
        8,5,9,7,6,1,4,2,3,
        4,2,6,8,5,3,7,9,1,
        7,1,3,9,2,4,8,5,6,
        9,6,1,5,3,7,2,8,4,
        2,8,7,4,1,9,6,3,5,
        3,4,5,2,8,6,1,7,9
    };

    for (auto _ : state) {
        benchmark::DoNotOptimize(SudokuValidator::isValidSolution(solved));
    }
}

static void BM_ValidatorHasConflicts(benchmark::State& state) {
    std::array<uint8_t, 81> solved = {
        5,3,4,6,7,8,9,1,2,
        6,7,2,1,9,5,3,4,8,
        1,9,8,3,4,2,5,6,7,
        8,5,9,7,6,1,4,2,3,
        4,2,6,8,5,3,7,9,1,
        7,1,3,9,2,4,8,5,6,
        9,6,1,5,3,7,2,8,4,
        2,8,7,4,1,9,6,3,5,
        3,4,5,2,8,6,1,7,9
    };

    for (auto _ : state) {
        benchmark::DoNotOptimize(SudokuValidator::hasConflicts(solved));
    }
}

BENCHMARK(BM_ValidatorValidSolution)->Name("Validator_ValidSolution");
BENCHMARK(BM_ValidatorHasConflicts)->Name("Validator_HasConflicts");

// Memory usage benchmark
static void BM_MemoryUsage(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<Sudoku> sudokus;
        state.ResumeTiming();

        // Allocate many Sudoku instances to measure memory usage
        for (int i = 0; i < state.range(0); ++i) {
            sudokus.emplace_back();
        }

        state.PauseTiming();
        sudokus.clear();
        state.ResumeTiming();
    }
}

BENCHMARK(BM_MemoryUsage)->Name("Memory_Usage")->Range(1000, 100000)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
