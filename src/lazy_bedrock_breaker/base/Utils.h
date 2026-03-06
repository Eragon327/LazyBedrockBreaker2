#pragma once

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/dimension/Dimension.h"
#include <cstdint>

namespace lazy_bedrock_breaker::utils {
struct AvilableBlockPos {
    int8_t direction;   // 0: below, 1: above, 2: north, 3: south, 4: west, 5: east, -1: none
    BlockPos blockPos;  // The position of the available block.
};

AvilableBlockPos getAvilabeBlockPos(BlockPos const& blockPos, Dimension const& dimension);
}    // namespace lazy_bedrock_breaker::utils