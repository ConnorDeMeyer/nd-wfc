#include <gtest/gtest.h>
#include <array>
#include <vector>
#include <algorithm>
#include <memory>
#include <span>
#include "nd-wfc/wfc.hpp"
#include "nd-wfc/worlds.hpp"
#include "nd-wfc/wfc_allocator.hpp"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
