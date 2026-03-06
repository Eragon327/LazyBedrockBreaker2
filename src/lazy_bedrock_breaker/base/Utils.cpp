#include "Utils.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/BlockSource.h"

namespace lazy_bedrock_breaker::utils {
AvilableBlockPos getAvilabeBlockPos(BlockPos const& blockPos, Dimension const& dimension) {
    BlockSource& blockSource = dimension.getBlockSourceFromMainChunkSource();

    if (blockSource.getBlock(blockPos.relative(0, 1)).isAir()) return AvilableBlockPos{0, blockPos.relative(0, 1)};
    if (blockSource.getBlock(blockPos.relative(1, 1)).isAir()) return AvilableBlockPos{1, blockPos.relative(1, 1)};
    if (blockSource.getBlock(blockPos.relative(2, 1)).isAir()) return AvilableBlockPos{2, blockPos.relative(2, 1)};
    if (blockSource.getBlock(blockPos.relative(3, 1)).isAir()) return AvilableBlockPos{3, blockPos.relative(3, 1)};
    if (blockSource.getBlock(blockPos.relative(4, 1)).isAir()) return AvilableBlockPos{4, blockPos.relative(4, 1)};
    if (blockSource.getBlock(blockPos.relative(5, 1)).isAir()) return AvilableBlockPos{5, blockPos.relative(5, 1)};

    return AvilableBlockPos{-1, blockPos}; // No available block position found.
}
}    // namespace lazy_bedrock_breaker::utils