
#pragma once

#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include <unordered_map>
#include <string>

namespace lazy_bedrock_breaker {
extern std::unordered_map<std::string, BlockPos>
    playerClickedPositions; // Map to store clicked block positions for each player.

extern std::string pistonFacingDirection[6];

void clearMap();

void onPlayerPlacingBlock(ll::event::PlayerPlacingBlockEvent& event);

void afterPlayerPlacedBlock(ll::event::PlayerPlacedBlockEvent& event);

bool getPistonReady(BlockSource& blockSource, const BlockPos& pos, const Block& piston);

unsigned char searchAvilablePistonFacingDirection(BlockSource& blockSource, const BlockPos& pos);

unsigned char handleDirection(unsigned char direction);

}