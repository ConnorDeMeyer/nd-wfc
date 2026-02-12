#include <nd-wfc/wfc.hpp>
#include <nd-wfc/wfc_builder.hpp>
#include <nd-wfc/worlds.hpp>
#include <nd-wfc/wfc_adjacency_matrix.hpp>
#include <iostream>
#include <string_view>

// --- Pattern to learn from ---
template <size_t N>
constexpr size_t maxRowWidth(const std::u32string_view (&rows)[N]) {
    size_t max = 0;
    for (auto& r : rows)
        if (r.size() > max) max = r.size();
    return max;
}

constexpr std::u32string_view patternRows[] = {
U"        ╔═╦═╗     ",
U"        ║H│.╠══╦═╗",
U"        ╚═╣.│..│T║",
U"          ║.╚══╬═╝",
U"  ╔═╗     ║...X║  ",
U"  ║.║    ╔╩╦═─╦╝  ",
U" ╔╩─╩╗   ║S│..║   ",
U" ║...╠═╦═╩─╩╦─╩═╗ ",
U"╔╝...║T│....│...║ ",
U"║..╔─╩╦╩═╦─═╩─═╦╝ ",
U"╚══╣..│..║....X║  ",
U"   ║X.║..│.....╠═╗",
U"   ╚╦─╬══╬══╗..│T║",
U"    ║W║  ║..│..╠═╝",
U"    ╚═╝  ║X.║..║  ",
U"         ╚══╬─╦╝  ",
U"            ║E║   ",
U"            ╚═╝   ",
};

constexpr size_t PatternWidth = maxRowWidth(patternRows);
constexpr size_t PatternHeight = std::size(patternRows);

using PatternWorld = WFC::Array2D<char32_t, PatternWidth, PatternHeight>;

// All unique tile types in the pattern
using IDMap = WFC::VariableIDMap<char32_t,
    U' ', U'.', U'║', U'═', U'╔', U'╗', U'╚', U'╝', U'╠', U'╣', U'╦', U'╩', U'╬', U'─', U'│', 'H', 'T', 'X', 'S', 'E', 'W'>;

using Adj = WFC::Array2DAdjacency<PatternWorld>;
using Matrix = WFC::AdjacencyMatrix<IDMap, Adj>;

// --- Output dungeon ---
constexpr size_t DungeonWidth = 16;
constexpr size_t DungeonHeight = 16;

using DungeonWorld = WFC::Array2D<char32_t, DungeonWidth, DungeonHeight>;
using DungeonAdj = WFC::Array2DAdjacency<DungeonWorld>;
using DungeonMatrix = WFC::AdjacencyMatrix<IDMap, DungeonAdj>;

using DungeonBuilder = WFC::Builder<DungeonWorld>
    ::DefineIDs<U' ', U'.', U'║', U'═', U'╔', U'╗', U'╚', U'╝', U'╠', U'╣', U'╦', U'╩', U'╬', U'─', U'│', 'H', 'T', 'X', 'S', 'E', 'W'>
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
    PatternWorld pattern{};
    for (size_t y = 0; y < PatternHeight; ++y)
        for (size_t x = 0; x < PatternWidth; ++x)
            pattern.setValue(y * PatternWidth + x, x < patternRows[y].size() ? patternRows[y][x] : U' ');

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
