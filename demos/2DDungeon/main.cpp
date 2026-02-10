#include <nd-wfc/wfc.hpp>
#include <nd-wfc/wfc_builder.hpp>
#include <nd-wfc/worlds.hpp>
#include <iostream>

constexpr size_t DungeonWidth = 16;
constexpr size_t DungeonHeight = 16;

using World = WFC::Array2D<char32_t, DungeonWidth, DungeonHeight>;

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

void printDungeon(const World& world) {
    for (size_t y = 0; y < DungeonHeight; ++y) {
        for (size_t x = 0; x < DungeonWidth; ++x) {
            char32_t tile = world.at(x, y);
            printChar32(tile);
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "2D Dungeon WFC Demo\n";
    std::cout << "Dungeon size: " << DungeonWidth << "x" << DungeonHeight << "\n\n";

    // Tile connection map:
    //   ' '  : none
    //   ╣    : up, down, left        ╠  : up, down, right
    //   ║    : up, down              ═  : left, right
    //   ╗    : down, left            ╚  : up, right
    //   ╝    : up, left              ╔  : down, right
    //   ╩    : up, left, right       ╦  : down, left, right
    //   ╬    : up, down, left, right
    //
    // Rule: adjacent tiles must agree on connections across their shared edge.

    using DungeonBuilder = WFC::Builder<World>
        ::DefineIDs<U'╣',U'╬'>
        ::Variable<U'╣'>
        ::Constrain<decltype([](World& world, size_t index, WFC::WorldValue<char32_t> val, auto& constrainer) constexpr {
            auto [x, y] = world.getCoord(index);
            
            // Down: must connect up → exclude tiles without up
            constrainer.template Exclude<U'╬'>(world.getCoordOffset(x, y, 0, 1));
        })>
        ::Variable<U'╬'>
        ::Constrain<decltype([](World& world, size_t index, WFC::WorldValue<char32_t> val, auto& constrainer) constexpr {
            auto [x, y] = world.getCoord(index);
            
            // Up: must connect down → exclude tiles without down
            constrainer.template Exclude<U'╣'>(world.getCoordOffset(x, y, 0, -1));
        })>
        ::Build;

    World world{};
    bool success = WFC::Run<DungeonBuilder>(world, std::random_device{}());
    if (!success) {
        std::cout << "WFC solver failed!\n";
    }
    printDungeon(world);

    return 0;
}
