#include <nd-wfc/wfc.hpp>
#include <nd-wfc/wfc_builder.hpp>
#include <nd-wfc/worlds.hpp>
#include <nd-wfc/wfc_adjacency_matrix.hpp>
#include <iostream>
#include <string_view>

// --- Pattern to learn from ---
constexpr size_t PatternWidth = 23;
constexpr size_t PatternHeight = 13;

using PatternWorld = WFC::Array2D<char32_t, PatternWidth, PatternHeight>;

// All unique tile types in the pattern
using IDMap = WFC::VariableIDMap<char32_t,
    U' ', U'║', U'═', U'╔', U'╗', U'╚', U'╝', U'╠', U'╣', U'╦', U'╩', U'╬'>;

using Adj = WFC::Array2DAdjacency<PatternWorld>;
using Matrix = WFC::AdjacencyMatrix<IDMap, Adj>;

// --- Output dungeon ---
constexpr size_t DungeonWidth = 30;
constexpr size_t DungeonHeight = 20;

using DungeonWorld = WFC::Array2D<char32_t, DungeonWidth, DungeonHeight>;
using DungeonAdj = WFC::Array2DAdjacency<DungeonWorld>;
using DungeonMatrix = WFC::AdjacencyMatrix<IDMap, DungeonAdj>;

using DungeonBuilder = WFC::Builder<DungeonWorld>
    ::DefineIDs<U' ', U'║', U'═', U'╔', U'╗', U'╚', U'╝', U'╠', U'╣', U'╦', U'╩', U'╬'>
    ::SetAdjacencyMatrix<DungeonMatrix, DungeonAdj>
    ::Build;

void printChar32(char32_t c) {
    char buf[4];
    int len = 0;
    if (c < 0x80) {
        buf[len++] = static_cast<char>(c);
    } else if (c < 0x800) {
        buf[len++] = static_cast<char>(0xC0 | (c >> 6));
        buf[len++] = static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        buf[len++] = static_cast<char>(0xE0 | (c >> 12));
        buf[len++] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        buf[len++] = static_cast<char>(0x80 | (c & 0x3F));
    } else {
        buf[len++] = static_cast<char>(0xF0 | (c >> 18));
        buf[len++] = static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        buf[len++] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        buf[len++] = static_cast<char>(0x80 | (c & 0x3F));
    }
    std::cout.write(buf, len);
}

template <typename WorldT>
void printWorld(const WorldT& world) {
    for (size_t y = 0; y < world.height(); ++y) {
        for (size_t x = 0; x < world.width(); ++x) {
            printChar32(world.at(x, y));
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "2D Dungeon WFC Demo (Adjacency Matrix)\n\n";

    // --- Step 1: Load the example pattern ---
    const std::u32string_view rows[] = {
        U"╔══════════╦══════════╗",
        U"║          ║          ║",
        U"║   ╔════╗ ║   ╔════╗ ║",
        U"║   ║    ║ ║   ║    ║ ║",
        U"╠═══╝    ╚═╬═══╝    ╚═╣",
        U"║          ║         ╔╝",
        U"║   ╔══╦═══╝   ╔══╦══╣",
        U"║   ║  ║       ║  ║  ║",
        U"╠═══╝  ╚═══════╝  ╚══╣",
        U"║                    ║",
        U"║   ╔════════════╗   ║",
        U"║   ║            ║   ║",
        U"╚═══╩════════════╩═══╝",
    };

    PatternWorld pattern{};
    for (size_t y = 0; y < PatternHeight; ++y)
        for (size_t x = 0; x < PatternWidth; ++x)
            pattern.setValue(y * PatternWidth + x, x < rows[y].size() ? rows[y][x] : U' ');

    std::cout << "Input pattern (" << PatternWidth << "x" << PatternHeight << "):\n";
    printWorld(pattern);

    // --- Step 2: Build adjacency matrix from the pattern ---
    Matrix patternMatrix;
    patternMatrix.BuildFromPattern(pattern);

    // Copy rules into a DungeonMatrix (same rules, different world size)
    DungeonMatrix dungeonMatrix;
    for (size_t dir = 0; dir < Matrix::AdjacencyCount; ++dir)
        for (size_t var = 0; var < IDMap::size(); ++var)
            dungeonMatrix.SetMask(dir, var, patternMatrix.GetMask(dir, var));

    // --- Step 3: Generate a new dungeon using the learned rules ---
    std::cout << "\nGenerated dungeon (" << DungeonWidth << "x" << DungeonHeight << "):\n";

    DungeonWorld dungeon{};
    bool success = WFC::Run<DungeonBuilder>(dungeon, std::random_device{}(), dungeonMatrix);

    if (!success) {
        std::cout << "WFC solver failed!\n";
        return 1;
    }

    printWorld(dungeon);
    return 0;
}
