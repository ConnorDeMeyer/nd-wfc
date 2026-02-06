#include "sudoku.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <bitset>

bool Sudoku::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::string puzzle_str;

    while (std::getline(file, line)) {
        // Remove whitespace and comments
        line.erase(std::remove_if(line.begin(), line.end(),
                   [](char c) { return std::isspace(c) || c == '#'; }), line.end());

        if (line.empty()) continue;
        puzzle_str += line;
    }

    return loadFromString(puzzle_str);
}

void Sudoku::print(char emptyVal) const {
    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            uint8_t value = get(row, col);
            std::cout << (value == 0 ? emptyVal : static_cast<char>('0' + value));
            if (col < 8) std::cout << " ";
            if (col == 2 || col == 5) std::cout << "| ";
        }
        std::cout << std::endl;
        if (row == 2 || row == 5) {
            std::cout << "------+-------+------" << std::endl;
        }
    }
}

std::string Sudoku::toString(char emptyVal) const {
    std::string result;
    result.reserve(81);
    for (int i = 0; i < 81; ++i) {
        uint8_t cell = board_.get(i);
        result += (cell == 0 ? emptyVal : static_cast<char>('0' + cell));
    }
    return result;
}

std::array<uint8_t, 81> Sudoku::getBoard() const {
    std::array<uint8_t, 81> result;
    for (int i = 0; i < 81; ++i) {
        result[i] = board_.get(i);
    }
    return result;
}





// SudokuValidator implementation
bool SudokuValidator::isValidSolution(const std::array<uint8_t, 81>& board) {
    // Check if all cells are filled and valid
    for (int i = 0; i < 81; ++i) {
        if (board[i] == 0) return false;
    }
    return isValidPartial(board);
}

bool SudokuValidator::isValidPartial(const std::array<uint8_t, 81>& board) {
    return !hasConflicts(board);
}

bool SudokuValidator::hasConflicts(const std::array<uint8_t, 81>& board) {
    // Check rows
    for (int row = 0; row < 9; ++row) {
        std::bitset<10> seen;
        for (int col = 0; col < 9; ++col) {
            uint8_t value = board[row * 9 + col];
            if (value != 0) {
                if (seen[value]) return true;
                seen.set(value);
            }
        }
    }

    // Check columns
    for (int col = 0; col < 9; ++col) {
        std::bitset<10> seen;
        for (int row = 0; row < 9; ++row) {
            uint8_t value = board[row * 9 + col];
            if (value != 0) {
                if (seen[value]) return true;
                seen.set(value);
            }
        }
    }

    // Check boxes
    for (int box = 0; box < 9; ++box) {
        std::bitset<10> seen;
        int startRow = (box / 3) * 3;
        int startCol = (box % 3) * 3;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                uint8_t value = board[(startRow + row) * 9 + (startCol + col)];
                if (value != 0) {
                    if (seen[value]) return true;
                    seen.set(value);
                }
            }
        }
    }

    return false;
}

std::vector<std::pair<int, int>> SudokuValidator::findConflicts(const std::array<uint8_t, 81>& board) {
    std::vector<std::pair<int, int>> conflicts;

    // Check rows
    for (int row = 0; row < 9; ++row) {
        std::bitset<10> seen;
        for (int col = 0; col < 9; ++col) {
            uint8_t value = board[row * 9 + col];
            if (value != 0) {
                if (seen[value]) {
                    conflicts.emplace_back(row, col);
                } else {
                    seen.set(value);
                }
            }
        }
    }

    // Check columns
    for (int col = 0; col < 9; ++col) {
        std::bitset<10> seen;
        for (int row = 0; row < 9; ++row) {
            uint8_t value = board[row * 9 + col];
            if (value != 0) {
                if (seen[value]) {
                    conflicts.emplace_back(row, col);
                } else {
                    seen.set(value);
                }
            }
        }
    }

    // Check boxes
    for (int box = 0; box < 9; ++box) {
        std::bitset<10> seen;
        int startRow = (box / 3) * 3;
        int startCol = (box % 3) * 3;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                int r = startRow + row;
                int c = startCol + col;
                uint8_t value = board[r * 9 + c];
                if (value != 0) {
                    if (seen[value]) {
                        conflicts.emplace_back(r, c);
                    } else {
                        seen.set(value);
                    }
                }
            }
        }
    }

    return conflicts;
}

// SudokuLoader implementation
std::optional<Sudoku> SudokuLoader::fromString(const std::string& puzzle_str) {
    Sudoku sudoku;
    if (sudoku.loadFromString(puzzle_str)) {
        return sudoku;
    }
    return std::nullopt;
}

std::optional<Sudoku> SudokuLoader::fromFile(const std::string& filename) {
    Sudoku sudoku;
    if (sudoku.loadFromFile(filename)) {
        return sudoku;
    }
    return std::nullopt;
}

std::vector<Sudoku> SudokuLoader::fromDirectory(const std::string& dirname, const std::string& extension) {
    std::vector<Sudoku> puzzles;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(dirname)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().string();
                if (filename.size() >= extension.size() &&
                    filename.substr(filename.size() - extension.size()) == extension) {
                    if (auto sudoku = fromFile(filename)) {
                        puzzles.push_back(std::move(*sudoku));
                    }
                }
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // Directory doesn't exist or can't be read
    }

    return puzzles;
}

bool SudokuLoader::parseLine(const std::string& line, std::array<uint8_t, 81>& board) {
    std::string cleaned = line;
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(),
                [](char c) { return std::isspace(c); }), cleaned.end());

    if (cleaned.length() != 81) return false;

    for (int i = 0; i < 81; ++i) {
        char c = cleaned[i];
        if (c >= '1' && c <= '9') {
            board[i] = c - '0';
        } else if (c == '0' || c == '.') {
            board[i] = 0;
        } else {
            return false;
        }
    }

    return true;
}