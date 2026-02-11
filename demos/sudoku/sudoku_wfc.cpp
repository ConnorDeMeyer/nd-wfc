#include <nd-wfc/wfc.hpp>
#include "sudoku.h"
#include <iostream>
#include <fstream>

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

using SudokuSolverCallback = SudokuSolverBuilder::SetCellCollapsedCallback<decltype([](const auto& state)
    {
        const auto& sudoku = state.m_world;
        static Sudoku LastSudoku{};
        static int counter = 0;
        for (size_t y = 0; y < 9; ++y) {
            for (size_t x = 0; x < 9; ++x) {
                int current = static_cast<int>(sudoku.getValue(x + y * 9));
                int last = static_cast<int>(LastSudoku.getValue(x + y * 9));
                if (current != last) {
                    std::cout << "\033[31m" << current << "\033[0m ";
                } else {
                    std::cout << current << " ";
                }
                if (x == 2 || x == 5) std::cout << "| ";
            }
            std::cout << std::endl;
            if (y == 2 || y == 5) std::cout << "------+-------+------" << std::endl;
        }
        LastSudoku = sudoku;

        std::cout << "Iteration: " << counter << std::endl;
        counter++;

        // std::cout << std::endl;
        // std::cout << "Press Enter to continue..." << std::endl;
        // std::cin.get();
    })>
    ::Build;

int main()
{
    std::cout << "Running Sudoku WFC" << std::endl;

    Sudoku sudokuWorld = Sudoku{ "6......3.......7....7463....7.8...2.4...9...1.9...7.8....9851....6.......1......9" };

    bool success = WFC::Run<SudokuSolverCallback>(sudokuWorld, true);

    bool solved = sudokuWorld.isSolved();

    if (success && solved) {
        std::cout << "Sudoku solved successfully!" << std::endl;
    } else {
        std::cout << "Failed to solve sudoku!" << std::endl;
    }

    // Print the solved sudoku
    for (size_t y = 0; y < 9; ++y) {
        for (size_t x = 0; x < 9; ++x) {
            std::cout << static_cast<int>(sudokuWorld.getValue(x + y * 9)) << " ";
            if (x == 2 || x == 5) std::cout << "| ";
        }
        std::cout << std::endl;
        if (y == 2 || y == 5) std::cout << "------+-------+------" << std::endl;
    }

    return (success && solved) ? 0 : 1;
}