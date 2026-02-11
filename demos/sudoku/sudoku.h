#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <array>
#include <chrono>
#include <bitset>
#include <algorithm>
#include <cctype>
#include <string_view>

#include <nd-wfc/wfc.h>

// 4-bit packed Sudoku board storage - optimal packing
// 81 cells * 4 bits = 324 bits
// Each byte holds 2 cells (8 bits / 4 bits per cell = 2)
// 81 cells / 2 = 40.5 bytes → 41 bytes total (with 4 bits unused)
class SudokuBoardStorage {
public:
    std::array<uint8_t, 41> data {};

    // Get 4-bit value at position (0-80)
    // Each byte contains 2 cells: [cell0(4bits)][cell1(4bits)]
    // Ultra-fast: only bitwise operations, no divide/modulo!
    // Optimization: pos >> 1 instead of pos / 2
    // Optimization: (pos & 1) << 2 instead of (pos % 2) * 4
    constexpr inline uint8_t get(int pos) const {
        int byteIndex = pos >> 1;  // pos / 2 using right shift

        // Precomputed shift amounts: 4 for even positions, 0 for odd positions
        // This is equivalent to: (4 - bitOffset) where bitOffset = (pos & 1) << 2
        // For even pos (0,2,4,...): 4 - 0 = 4
        // For odd pos (1,3,5,...): 4 - 4 = 0
        int shiftAmount = 4 - ((pos & 1) << 2);

        uint8_t result = (data[byteIndex] >> shiftAmount) & 0xF;

        // Debug assertion: ensure result is in valid range
        WFC::constexpr_assert(result <= 9, "Sudoku cell value must be between 0-9");

        return result;
    }

    // Set 4-bit value at position (0-80)
    // Ultra-fast: only bitwise operations, no divide/modulo!
    // Optimization: pos >> 1 instead of pos / 2
    // Optimization: (pos & 1) << 2 instead of (pos % 2) * 4
    constexpr inline void set(int pos, uint8_t value) {
        // Assert that value is in valid Sudoku range (0-9)
        WFC::constexpr_assert(value <= 9, "Sudoku cell value must be between 0-9");

        int byteIndex = pos >> 1;  // pos / 2 using right shift

        // Precomputed shift amounts: 4 for even positions, 0 for odd positions
        int shiftAmount = 4 - ((pos & 1) << 2);

        // Create mask to clear the 4 bits we're setting
        uint8_t mask = ~(0xF << shiftAmount);

        // Set the value (value is already 0-9, so only lower 4 bits are used)
        data[byteIndex] = (data[byteIndex] & mask) | (value << shiftAmount);
    }

    constexpr void clear() {
        data.fill(0);
    }
};

// Ultra-memory-efficient Sudoku class: exactly 41 bytes
class Sudoku {
public:
    constexpr Sudoku() = default;
    constexpr explicit Sudoku(std::string_view puzzle_str)
    {
        loadFromString(puzzle_str);
    }

    // Load from various formats
    constexpr bool loadFromString(std::string_view puzzle_str)
    {
        if (puzzle_str.length() != 81) {
            return false;
        }
    
        clear();
    
        for (int i = 0; i < 81; ++i) {
            char c = puzzle_str[i];
            if (c >= '1' && c <= '9') {
                int row = i / 9;
                int col = i % 9;
                uint8_t value = c - '0';
                if (!set(row, col, value)) {
                    return false; // Invalid move
                }
            } else if (c != '0' && c != '.') {
                return false; // Invalid character
            }
        }
    
        return true;
    }
    bool loadFromFile(const std::string& filename);

    // Board access (inlined for performance)
    constexpr inline uint8_t get(int row, int col) const {
        WFC::constexpr_assert((row >= 0 && row < 9 && col >= 0 && col < 9),
               "Sudoku::get() called with invalid position - row and col must be 0-8");
        int linearIndex = getLinearIndex(row, col);
        return board_.get(linearIndex);
    }

    constexpr inline bool set(int row, int col, uint8_t value) {
        WFC::constexpr_assert((row >= 0 && row < 9 && col >= 0 && col < 9),
               "Sudoku::set() called with invalid position - row and col must be 0-8");

        // Keep value validation as runtime check since it's about valid Sudoku numbers
        if (value > 9) return false;

        int linearIndex = getLinearIndex(row, col);
        uint8_t old_value = board_.get(linearIndex);

        // If same value, no change needed
        if (old_value == value) return true;

        // Check if new value is valid (skip for 0 as it clears)
        if (value != 0 && !isValidMove(row, col, value)) {
            return false;
        }

        board_.set(linearIndex, value);
        return true;
    }

    constexpr void clear() {
        board_.clear();
    }

    // Validation
    constexpr bool isValid() const {
        // Check rows for duplicates using bitset for efficiency
        for (int row = 0; row < 9; ++row) {
            std::bitset<10> seen; // bits 1-9 track values 1-9
            for (int col = 0; col < 9; ++col) {
                uint8_t value = get(row, col);
                if (value != 0) {
                    if (seen[value]) return false; // Duplicate found
                    seen.set(value);
                }
            }
        }
    
        // Check columns for duplicates
        for (int col = 0; col < 9; ++col) {
            std::bitset<10> seen;
            for (int row = 0; row < 9; ++row) {
                uint8_t value = get(row, col);
                if (value != 0) {
                    if (seen[value]) return false; // Duplicate found
                    seen.set(value);
                }
            }
        }
    
        // Check boxes for duplicates
        for (int box = 0; box < 9; ++box) {
            std::bitset<10> seen;
            int startRow = (box / 3) * 3;
            int startCol = (box % 3) * 3;
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    uint8_t value = get(startRow + row, startCol + col);
                    if (value != 0) {
                        if (seen[value]) return false; // Duplicate found
                        seen.set(value);
                    }
                }
            }
        }
    
        return true;
    }

    constexpr bool isSolved() const {
        for (int i = 0; i < 81; ++i) {
            if (board_.get(i) == 0) return false;
        }
        return isValid();
    }

    // Inlined validation (called frequently from set())
    constexpr inline bool isValidMove(int row, int col, uint8_t value) const {
        if (value == 0 || value > 9) return false;
        return !hasRowConflictExcluding(row, col, value) &&
               !hasColConflictExcluding(col, row, value) &&
               !hasBoxConflictExcluding(getBoxIndex(row, col), row, col, value);
    }

    // Utility
    void print(char emptyVal = '.') const;
    std::string toString(char emptyVal = '.') const;

    // Convert to standard board format for external use
    std::array<uint8_t, 81> getBoard() const;

private:
    SudokuBoardStorage board_;

    // Helper functions (inlined for performance)
    constexpr inline int getLinearIndex(int row, int col) const {
        return row * 9 + col;
    }

    constexpr inline int getBoxIndex(int row, int col) const {
        return (row / 3) * 3 + (col / 3);
    }

    constexpr inline bool isValidPosition(int row, int col) const {
        return row >= 0 && row < 9 && col >= 0 && col < 9;
    }

    // Validation helpers (inlined for performance)
    // Uses std::bitset<10> for efficient duplicate detection instead of arrays
    constexpr inline bool hasRowConflict(int row, uint8_t value) const {
        for (int col = 0; col < 9; ++col) {
            if (get(row, col) == value) return true;
        }
        return false;
    }

    constexpr inline bool hasColConflict(int col, uint8_t value) const {
        for (int row = 0; row < 9; ++row) {
            if (get(row, col) == value) return true;
        }
        return false;
    }

    constexpr inline bool hasBoxConflict(int box, uint8_t value) const {
        int startRow = (box / 3) * 3;
        int startCol = (box % 3) * 3;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                if (get(startRow + row, startCol + col) == value) return true;
            }
        }
        return false;
    }

    // Validation helpers that exclude current position (for move validation)
    constexpr inline bool hasRowConflictExcluding(int row, int excludeCol, uint8_t value) const {
        for (int col = 0; col < 9; ++col) {
            if (col != excludeCol && get(row, col) == value) return true;
        }
        return false;
    }

    constexpr inline bool hasColConflictExcluding(int col, int excludeRow, uint8_t value) const {
        for (int row = 0; row < 9; ++row) {
            if (row != excludeRow && get(row, col) == value) return true;
        }
        return false;
    }

    constexpr inline bool hasBoxConflictExcluding(int box, int excludeRow, int excludeCol, uint8_t value) const {
        int startRow = (box / 3) * 3;
        int startCol = (box % 3) * 3;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                int r = startRow + row;
                int c = startCol + col;
                if ((r != excludeRow || c != excludeCol) && get(r, c) == value) return true;
            }
        }
        return false;
    }

public: // WFC Support
    using ValueType = uint8_t;

    constexpr inline ValueType getValue(uint8_t index) const {
        return board_.get(static_cast<int>(index));
    }

    constexpr inline void setValue(uint8_t index, ValueType value) {
        board_.set(static_cast<int>(index), value);
    }

    constexpr inline uint8_t size() const {
        return 81;
    }
};

// Static assert to ensure correct size (now 56 bytes with solver additions)
static_assert(sizeof(Sudoku) == 41, "Sudoku class must be exactly 41 bytes");
static_assert(WFC::HasConstexprSize<Sudoku>, "Sudoku class must have a constexpr size() method");

// Fast solution validator (stateless)
class SudokuValidator {
public:
    static bool isValidSolution(const std::array<uint8_t, 81>& board);
    static bool isValidPartial(const std::array<uint8_t, 81>& board);
    static bool hasConflicts(const std::array<uint8_t, 81>& board);
    static std::vector<std::pair<int, int>> findConflicts(const std::array<uint8_t, 81>& board);
};

// Fast puzzle loader
class SudokuLoader {
public:
    static std::optional<Sudoku> fromString(const std::string& puzzle_str);
    static std::optional<Sudoku> fromFile(const std::string& filename);
    static std::vector<Sudoku> fromDirectory(const std::string& dirname, const std::string& extension = ".txt");

private:
    static bool parseLine(const std::string& line, std::array<uint8_t, 81>& board);
};

using SudokuSolverBuilder = WFC::Builder<Sudoku>
    ::DefineRange<1, 10>
    ::ConstrainAll<decltype([](const Sudoku&, size_t index, WFC::WorldValue<uint8_t> val, auto& constrainer) constexpr {
        size_t x = index % 9;
        size_t y = index / 9;
        
        for (size_t i = 0; i < 9; ++i) 
        {
            // Add row constraints (same row, different columns)
            if (i != x) constrainer.Exclude(val, i + y * 9);
            // Add column constraints (same column, different rows)
            if (i != y) constrainer.Exclude(val, x + i * 9);
        }

        // Add box constraints (3x3 box)
        size_t box_x = (x / 3) * 3;
        size_t box_y = (y / 3) * 3;
        for (size_t j = 0; j < 3; ++j) {
            for (size_t k = 0; k < 3; ++k) {
                size_t cell_x = box_x + j;
                size_t cell_y = box_y + k;
                size_t cell_index = cell_x + cell_y * 9;
                if (cell_index != index) {
                    constrainer.Exclude(val, cell_index);
                }
            }
        }
        
    })>;

using SudokuSolver = SudokuSolverBuilder::Build;

