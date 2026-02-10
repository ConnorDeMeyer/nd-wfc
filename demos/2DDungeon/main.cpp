#include <nd-wfc/wfc.hpp>
#include <nd-wfc/wfc_builder.hpp>
#include <nd-wfc/worlds.hpp>
#include <iostream>

// Tile types for the dungeon
// Values start at 1 so default-initialized cells (0) don't match any tile
enum class Tile : int {
    Empty = 1,
    Wall = 2,
    Floor = 3
};

constexpr size_t DungeonWidth = 16;
constexpr size_t DungeonHeight = 16;

using World = WFC::Array2D<Tile, DungeonWidth, DungeonHeight>;

// Concrete types needed for constrainer lambda signatures
using IDMap = WFC::VariableIDMap<Tile, Tile::Floor, Tile::Wall, Tile::Empty>;
constexpr size_t WorldSize = DungeonWidth * DungeonHeight;
using WaveT = WFC::Wave<IDMap, WorldSize>;
using QueueT = WFC::WFCQueue<WorldSize, size_t>;
using ConstrainerT = WFC::Constrainer<WaveT, QueueT>;

void printDungeon(const World& world) {
    for (size_t y = 0; y < DungeonHeight; ++y) {
        for (size_t x = 0; x < DungeonWidth; ++x) {
            switch (world.at(x, y)) {
                case Tile::Floor:    std::cout << '.'; break;
                case Tile::Wall:     std::cout << '#'; break;
                case Tile::Empty:    std::cout << ' '; break;
            }
        }
        std::cout << '\n';
    }
}

int main() {
    std::cout << "2D Dungeon WFC Demo\n";
    std::cout << "Dungeon size: " << DungeonWidth << "x" << DungeonHeight << "\n\n";

    using DungeonBuilder = WFC::Builder<World>
        ::DefineIDs<Tile::Floor, Tile::Wall, Tile::Empty>
        ::Variable<Tile::Floor>
        ::Constrain<decltype([](World& world, size_t index, WFC::WorldValue<Tile> val, ConstrainerT& constrainer) {

            auto [x, y] = world.getCoord(index);

            // enable walls in 3x3 area around floor (without center)
            // must come before Exclude to avoid collapsing then un-collapsing cells
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    constrainer.template Include<Tile::Wall>(world.getCoordOffset(x, y, dx, dy));
                }
            }

            // floor cannot be adjacent to empty space
            constrainer.template Exclude<Tile::Empty>(world.getCoordOffset(x, y, -1, 0)); // Left
            constrainer.template Exclude<Tile::Empty>(world.getCoordOffset(x, y, 1, 0));  // Right
            constrainer.template Exclude<Tile::Empty>(world.getCoordOffset(x, y, 0, -1)); // Up
            constrainer.template Exclude<Tile::Empty>(world.getCoordOffset(x, y, 0, 1));  // Down
        })>
        ::SetInitialState<decltype([](World& world, auto& constrainer, auto&) constexpr {
            // disable walls everywhere by default
            for (size_t i = 0; i < world.size(); ++i) {
                constrainer.template Exclude<Tile::Wall>(i);
            }
            // make it impossible for the edge to be floor
            for (size_t x = 0; x < world.width(); ++x) {
                constrainer.template Exclude<Tile::Floor>(world.getId({static_cast<int>(x), 0}));
                constrainer.template Exclude<Tile::Floor>(world.getId({static_cast<int>(x), static_cast<int>(world.height() - 1)}));
            }
            // seed floor tiles to kick-start dungeon generation
            constrainer.template Only<Tile::Floor>(world.getId({2, 2}));
        })>
        ::SetRandomSelector<WFC::AdvancedRandomSelector<Tile>>
        ::Build;

    World world{};
    bool success = WFC::Run<DungeonBuilder>(world, std::random_device{}());
    if (!success) {
        std::cout << "WFC solver failed!\n";
    }
    printDungeon(world);

    return 0;
}
