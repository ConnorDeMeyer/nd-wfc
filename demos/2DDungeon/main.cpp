#include <nd-wfc/wfc.hpp>
#include <nd-wfc/wfc_builder.hpp>
#include <nd-wfc/worlds.hpp>
#include <iostream>
#include <string>
#include <array>
#include <filesystem>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// Tile types for the dungeon
// Values start at 1 so default-initialized cells (0) don't match any tile
enum class Tile : int {
    Empty = 1,
    Wall = 2,
    Floor = 3
};

constexpr size_t DungeonWidth = 16;
constexpr size_t DungeonHeight = 16;
constexpr int TileSize = 16; // pixel size of each tile on screen
constexpr int NumTileImages = 25;

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

std::string getTilesPath(const char* argv0) {
    // Try relative to executable first
    auto execDir = std::filesystem::path(argv0).parent_path();
    auto candidate = execDir / "../../data/tiles";
    if (std::filesystem::exists(candidate))
        return candidate.string();

    // Try relative to CWD
    candidate = "data/tiles";
    if (std::filesystem::exists(candidate))
        return candidate.string();

    // Try absolute fallback
    candidate = std::filesystem::path(__FILE__).parent_path() / "data/tiles";
    if (std::filesystem::exists(candidate))
        return candidate.string();

    return "data/tiles";
}

int main(int argc, char* argv[]) {
    (void)argc;

    std::cout << "2D Dungeon WFC Demo\n";
    std::cout << "Dungeon size: " << DungeonWidth << "x" << DungeonHeight << "\n\n";

    // --- Run WFC ---
    using DungeonBuilder = WFC::Builder<World>
        ::DefineIDs<Tile::Floor, Tile::Wall, Tile::Empty>
        ::Variable<Tile::Floor>
        ::Constrain<decltype([](World& world, size_t index, WFC::WorldValue<Tile> val, ConstrainerT& constrainer) {

            auto [x, y] = world.getCoord(index);

            // enable walls in 3x3 area around floor (without center)
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    constrainer.template Include<Tile::Wall>(world.getCoordOffset(x, y, dx, dy));
                }
            }

            // floor cannot be adjacent to empty space
            constrainer.template Exclude<Tile::Empty>(world.getCoordOffset(x, y, -1, 0));
            constrainer.template Exclude<Tile::Empty>(world.getCoordOffset(x, y, 1, 0));
            constrainer.template Exclude<Tile::Empty>(world.getCoordOffset(x, y, 0, -1));
            constrainer.template Exclude<Tile::Empty>(world.getCoordOffset(x, y, 0, 1));
        })>
        ::SetInitialState<decltype([](World& world, auto& constrainer, auto&) constexpr {
            for (size_t i = 0; i < world.size(); ++i) {
                constrainer.template Exclude<Tile::Wall>(i);
            }
            for (size_t x = 0; x < world.width(); ++x) {
                constrainer.template Exclude<Tile::Floor>(world.getId({static_cast<int>(x), 0}));
                constrainer.template Exclude<Tile::Floor>(world.getId({static_cast<int>(x), static_cast<int>(world.height() - 1)}));
            }
            constrainer.template Only<Tile::Floor>(world.getId({2, 2}));
        })>
        ::SetRandomSelector<WFC::AdvancedRandomSelector<Tile>>
        ::Build;

    World world{};
    bool success = WFC::Run<DungeonBuilder>(world, std::random_device{}());
    if (!success) {
        std::cout << "WFC solver failed!\n";
        return 1;
    }
    printDungeon(world);

    // --- SDL2 rendering ---
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
        return 1;
    }

    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "IMG_Init error: " << IMG_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    constexpr int windowW = static_cast<int>(DungeonWidth) * TileSize;
    constexpr int windowH = static_cast<int>(DungeonHeight) * TileSize;

    SDL_Window* window = SDL_CreateWindow(
        "2D Dungeon WFC Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowW, windowH,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow error: " << SDL_GetError() << "\n";
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load tile textures
    std::string tilesPath = getTilesPath(argv[0]);
    std::cout << "Loading tiles from: " << tilesPath << "\n";

    std::array<SDL_Texture*, NumTileImages> tileTextures{};
    for (int i = 0; i < NumTileImages; ++i) {
        char filename[32];
        snprintf(filename, sizeof(filename), "tile_%04d.png", i);
        std::string fullPath = tilesPath + "/" + filename;

        SDL_Surface* surface = IMG_Load(fullPath.c_str());
        if (!surface) {
            std::cerr << "Failed to load " << fullPath << ": " << IMG_GetError() << "\n";
            tileTextures[i] = nullptr;
            continue;
        }
        tileTextures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    // Simple mapping from Tile enum to a tile image index
    // Adjust these indices to match whichever tile images look best
    auto getTileIndex = [](Tile t) -> int {
        switch (t) {
            case Tile::Floor: return 1;  // tile_0001.png
            case Tile::Wall:  return 0;  // tile_0000.png
            case Tile::Empty: return 2;  // tile_0002.png
        }
        return 0;
    };

    // Main loop
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                // Regenerate dungeon on spacebar
                World newWorld{};
                if (WFC::Run<DungeonBuilder>(newWorld, std::random_device{}())) {
                    world = newWorld;
                    std::cout << "\nRegenerated dungeon:\n";
                    printDungeon(world);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render the dungeon
        for (size_t y = 0; y < DungeonHeight; ++y) {
            for (size_t x = 0; x < DungeonWidth; ++x) {
                int tileIdx = getTileIndex(world.at(x, y));
                SDL_Texture* tex = tileTextures[tileIdx];
                if (tex) {
                    SDL_Rect dst = {
                        static_cast<int>(x) * TileSize,
                        static_cast<int>(y) * TileSize,
                        TileSize,
                        TileSize
                    };
                    SDL_RenderCopy(renderer, tex, nullptr, &dst);
                }
            }
        }

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    for (auto* tex : tileTextures) {
        if (tex) SDL_DestroyTexture(tex);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
