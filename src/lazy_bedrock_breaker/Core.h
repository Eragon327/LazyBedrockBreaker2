#pragma once

#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "mc/platform/UUID.h"
#include "mc/world/level/BlockPos.h"
#include <unordered_map>


namespace lazy_bedrock_breaker::core {
extern std::unordered_map<mce::UUID, BlockPos>
    playerClickedPositions; // Map to store clicked block positions for each player.

void clearMap();

void onPlayerPlacingBlock(ll::event::PlayerPlacingBlockEvent& event);

void afterPlayerPlacedBlock(ll::event::PlayerPlacedBlockEvent& event);

bool clientPlaceBlock(const Player& clientPlayer, const BlockPos& placePos, int slot, uchar face);
}