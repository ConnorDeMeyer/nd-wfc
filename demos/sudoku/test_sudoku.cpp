#include <gtest/gtest.h>
#include "sudoku.h"
#include <chrono>
#include <nd-wfc/wfc.hpp>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>

// Forward declarations for helper functions
std::vector<Sudoku> loadPuzzlesFromFile(const std::string& filename);
void testPuzzleSolving(const std::string& difficulty, const std::string& filename);

// Test fixture for Sudoku tests
class SudokuTest : public ::testing::Test {
protected:
    // Helper function to create a solved Sudoku
    Sudoku createSolvedSudoku() {
        Sudoku sudoku;
        std::string solved = "534678912672195348198342567859761423426853791713924856961537284287419635345286179";
        sudoku.loadFromString(solved);
        return sudoku;
    }

    // Helper function to create an easy puzzle
    Sudoku createEasyPuzzle() {
        Sudoku sudoku;
        std::string easy = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";
        sudoku.loadFromString(easy);
        return sudoku;
    }

    // Helper function to solve a puzzle using WFC
    void solvePuzzle(Sudoku& sudoku) {
        SudokuSolver::Run(sudoku, true);
    }
};

// Basic functionality tests
TEST_F(SudokuTest, EmptySudoku) {
    Sudoku sudoku;

    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            EXPECT_EQ(sudoku.get(row, col), 0);
        }
    }
}

TEST_F(SudokuTest, SetAndGet) {
    Sudoku sudoku;

    // Test setting and getting values
    sudoku.set(0, 0, 5);
    EXPECT_EQ(sudoku.get(0, 0), 5);

    sudoku.set(8, 8, 9);
    EXPECT_EQ(sudoku.get(8, 8), 9);

    sudoku.set(4, 4, 7);
    EXPECT_EQ(sudoku.get(4, 4), 7);
}

TEST_F(SudokuTest, LoadFromString) {
    Sudoku sudoku;
    std::string puzzle = "530070000600195000098000060800060003400803001700020006060000280000419005000080079";

    EXPECT_TRUE(sudoku.loadFromString(puzzle));

    // Verify specific known values
    EXPECT_EQ(sudoku.get(0, 0), 5);
    EXPECT_EQ(sudoku.get(0, 1), 3);
    EXPECT_EQ(sudoku.get(0, 6), 0); // Empty cell
}

TEST_F(SudokuTest, LoadInvalidString) {
    Sudoku sudoku;

    // Test with string that's too short
    EXPECT_FALSE(sudoku.loadFromString("123"));

    // Test with invalid characters
    EXPECT_FALSE(sudoku.loadFromString("53007000060019500009800006080006000340080300170002000606000028000041900500008007a"));
}

TEST_F(SudokuTest, Clear) {
    Sudoku sudoku;
    sudoku.set(0, 0, 5);
    sudoku.set(1, 1, 3);
    sudoku.set(2, 2, 7);

    EXPECT_EQ(sudoku.get(0, 0), 5);
    EXPECT_EQ(sudoku.get(1, 1), 3);
    EXPECT_EQ(sudoku.get(2, 2), 7);

    sudoku.clear();

    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            EXPECT_EQ(sudoku.get(row, col), 0);
        }
    }
}

TEST_F(SudokuTest, MemorySize) {
    EXPECT_EQ(sizeof(Sudoku), 41);
}

TEST_F(SudokuTest, SetInvalidValue) {
    Sudoku sudoku;

    // Test setting value > 9
    EXPECT_FALSE(sudoku.set(0, 0, 10));
    EXPECT_FALSE(sudoku.set(0, 0, 15));
    EXPECT_FALSE(sudoku.set(0, 0, 255));

    // Valid values should work
    EXPECT_TRUE(sudoku.set(0, 0, 9));
    EXPECT_EQ(sudoku.get(0, 0), 9);
}

// Validation tests
TEST_F(SudokuTest, ValidMoves) {
    auto sudoku = createEasyPuzzle();

    // Test valid moves - place numbers where there are empty cells
    EXPECT_TRUE(sudoku.set(0, 2, 1)); // Should be valid - empty cell
    EXPECT_EQ(sudoku.get(0, 2), 1);

    EXPECT_TRUE(sudoku.set(1, 1, 4)); // Should be valid - empty cell
    EXPECT_EQ(sudoku.get(1, 1), 4);

    EXPECT_TRUE(sudoku.set(2, 0, 2)); // Should be valid - empty cell, different value
    EXPECT_EQ(sudoku.get(2, 0), 2);
}

TEST_F(SudokuTest, InvalidMoves) {
    auto sudoku = createEasyPuzzle();

    // Test moves that conflict with existing numbers
    EXPECT_FALSE(sudoku.set(0, 0, 6)); // Conflicts with existing 5 in row
    EXPECT_FALSE(sudoku.set(0, 1, 6)); // Conflicts with existing 3 in column (try different value)
    EXPECT_FALSE(sudoku.set(2, 2, 9)); // Conflicts with existing 8 in same box

    // Test setting same value as existing (should work)
    EXPECT_TRUE(sudoku.set(0, 0, 5));  // Same as existing value
    EXPECT_EQ(sudoku.get(0, 0), 5);
    EXPECT_TRUE(sudoku.set(0, 1, 3));  // Same as existing value
    EXPECT_EQ(sudoku.get(0, 1), 3);
}

TEST_F(SudokuTest, SolvedPuzzle) {
    auto sudoku = createSolvedSudoku();

    EXPECT_TRUE(sudoku.isValid());
    EXPECT_TRUE(sudoku.isSolved());
}

TEST_F(SudokuTest, PartialPuzzle) {
    auto sudoku = createEasyPuzzle();

    EXPECT_TRUE(sudoku.isValid());
    EXPECT_FALSE(sudoku.isSolved());
}

// Board conversion tests
TEST_F(SudokuTest, GetBoard) {
    auto sudoku = createEasyPuzzle();
    auto board = sudoku.getBoard();

    EXPECT_EQ(board.size(), 81);
    EXPECT_EQ(board[0], 5);  // First cell
    EXPECT_EQ(board[1], 3);  // Second cell
}

TEST_F(SudokuTest, ToString) {
    Sudoku sudoku;
    sudoku.set(0, 0, 5);
    sudoku.set(0, 1, 3);

    std::string str = sudoku.toString();
    EXPECT_EQ(str.length(), 81);
    EXPECT_EQ(str[0], '5');
    EXPECT_EQ(str[1], '3');
}

// Validator tests
TEST_F(SudokuTest, ValidatorValidSolution) {
    auto sudoku = createSolvedSudoku();
    auto board = sudoku.getBoard();

    EXPECT_TRUE(SudokuValidator::isValidSolution(board));
    EXPECT_TRUE(SudokuValidator::isValidPartial(board));
    EXPECT_FALSE(SudokuValidator::hasConflicts(board));
}

TEST_F(SudokuTest, ValidatorInvalidSolution) {
    auto sudoku = createSolvedSudoku();
    auto board = sudoku.getBoard();

    // Create a conflict
    board[1] = 5; // This creates duplicate 5 in first row

    EXPECT_FALSE(SudokuValidator::isValidSolution(board));
    EXPECT_FALSE(SudokuValidator::isValidPartial(board));
    EXPECT_TRUE(SudokuValidator::hasConflicts(board));

    auto conflicts = SudokuValidator::findConflicts(board);
    EXPECT_GT(conflicts.size(), 0);
}

// Performance tests
TEST_F(SudokuTest, PerformanceGetOperations) {
    auto sudoku = createEasyPuzzle();

    // Time 100,000 get operations
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; ++i) {
        int row = i % 9;
        int col = (i / 9) % 9;
        volatile uint8_t value = sudoku.get(row, col);
        (void)value; // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "100,000 get operations took: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Average per operation: " << (duration.count() / 100000.0) << " microseconds" << std::endl;
}

TEST_F(SudokuTest, PerformanceSetOperations) {
    Sudoku sudoku;

    // Time 100,000 set operations
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; ++i) {
        int row = i % 9;
        int col = (i / 9) % 9;
        int value = (i % 9) + 1;
        sudoku.set(row, col, static_cast<uint8_t>(value));
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "100,000 set operations took: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Average per operation: " << (duration.count() / 100000.0) << " microseconds" << std::endl;
}

// Edge case tests
TEST_F(SudokuTest, EdgeCases) {
    Sudoku sudoku;

    // Test all edge positions
    EXPECT_TRUE(sudoku.set(0, 0, 1)); // Top-left
    EXPECT_TRUE(sudoku.set(0, 8, 2)); // Top-right
    EXPECT_TRUE(sudoku.set(8, 0, 3)); // Bottom-left
    EXPECT_TRUE(sudoku.set(8, 8, 4)); // Bottom-right

    EXPECT_EQ(sudoku.get(0, 0), 1);
    EXPECT_EQ(sudoku.get(0, 8), 2);
    EXPECT_EQ(sudoku.get(8, 0), 3);
    EXPECT_EQ(sudoku.get(8, 8), 4);
}

TEST_F(SudokuTest, WFCIntegration) {
    Sudoku sudoku = createEasyPuzzle();
    solvePuzzle(sudoku);
    EXPECT_TRUE(sudoku.isSolved());
}

// Unified function to test puzzle solving for different difficulty levels
void testPuzzleSolving(const std::string& difficulty, const std::string& filename) {
    std::vector<Sudoku> puzzles = loadPuzzlesFromFile(filename);

    ASSERT_GT(puzzles.size(), 0) << "No " << difficulty << " puzzles loaded";

    int solvedCount = 0;
    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < puzzles.size(); ++i) {
        Sudoku& sudoku = puzzles[i];
        EXPECT_TRUE(sudoku.isValid()) << difficulty << " puzzle " << i << " is not valid";

        SudokuSolver::Run(sudoku);

        EXPECT_TRUE(sudoku.isSolved()) << difficulty << " puzzle " << i << " was not solved. Puzzle string: " << sudoku.toString();

        if (sudoku.isSolved()) {
            solvedCount++;
        }

        // give progress every max(100, puzzles.size() / 100) puzzles and output percentage complete
        if (i % std::max<size_t>(100, puzzles.size() / 100) == 0) {
            std::cout << difficulty << " puzzles: solved " << solvedCount << "/" << puzzles.size()
                      << " in " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() << " seconds" << std::endl;
            std::cout << "Percentage complete: " << static_cast<int>(static_cast<double>(i) / puzzles.size() * 100.0) << "%" << std::endl;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    std::cout << difficulty << " puzzles: solved " << solvedCount << "/" << puzzles.size()
              << " in " << totalDuration.count() << " seconds" << std::endl;

    EXPECT_EQ(solvedCount, puzzles.size()) << "Not all " << difficulty << " puzzles were solved";
}

// Tests loading and solving puzzles from data files using the unified function
TEST_F(SudokuTest, LoadAndSolveEasyPuzzles) {
    testPuzzleSolving("Easy", "../data/Sudoku_easy.txt");
}

TEST_F(SudokuTest, LoadAndSolveMediumPuzzles) {
    testPuzzleSolving("Medium", "../data/Sudoku_medium.txt");
}

TEST_F(SudokuTest, LoadAndSolveHardPuzzles) {
    testPuzzleSolving("Hard", "../data/Sudoku_hard.txt");
}

TEST_F(SudokuTest, LoadAndSolveDiabolicalPuzzles) {
    testPuzzleSolving("Diabolical", "../data/Sudoku_diabolical.txt");
}

// Test loading and solving the first puzzle from each difficulty file
TEST_F(SudokuTest, LoadAndSolveFirstPuzzleFromEachFile) {
    const std::string dataPath = "../data";
    const std::vector<std::pair<std::string, std::string>> fileTests = {
        {"Sudoku_easy.txt", "Easy"},
        {"Sudoku_medium.txt", "Medium"},
        {"Sudoku_hard.txt", "Hard"},
        {"Sudoku_diabolical.txt", "Diabolical"}
    };

    for (const auto& [filename, difficulty] : fileTests) {
        std::string filepath = dataPath + "/" + filename;
        std::ifstream file(filepath);

        ASSERT_TRUE(file.is_open()) << "Failed to open file " << filename;

        std::string firstLine;
        std::getline(file, firstLine);

        // Remove whitespace
        firstLine.erase(std::remove_if(firstLine.begin(), firstLine.end(),
                       [](char c) { return std::isspace(c); }), firstLine.end());

        ASSERT_FALSE(firstLine.empty()) << "No puzzle data found in " << filename;

        Sudoku puzzle;
        ASSERT_TRUE(puzzle.loadFromString(firstLine)) << "Failed to load puzzle from first line of " << filename;

        EXPECT_TRUE(puzzle.isValid()) << "Loaded puzzle from " << filename << " is not valid";

        auto start = std::chrono::high_resolution_clock::now();
        solvePuzzle(puzzle);
        auto end = std::chrono::high_resolution_clock::now();

        EXPECT_TRUE(puzzle.isSolved()) << "Failed to solve first puzzle from " << filename;

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "First " << difficulty << " puzzle solved in " << duration.count() << "ms" << std::endl;
    }
}

// Helper function to load multiple puzzles from a file (one puzzle per line)
std::vector<Sudoku> loadPuzzlesFromFile(const std::string& filename) {
    std::vector<Sudoku> puzzles;
    std::ifstream file(filename);

    if (!file.is_open()) {
        return puzzles;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remove whitespace
        line.erase(std::remove_if(line.begin(), line.end(),
                   [](char c) { return std::isspace(c); }), line.end());

        if (line.empty()) continue;

        Sudoku sudoku;
        if (sudoku.loadFromString(line)) {
            puzzles.push_back(std::move(sudoku));
        }
    }

    return puzzles;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
