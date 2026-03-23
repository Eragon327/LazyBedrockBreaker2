
#pragma once

#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include <optional>
#include <unordered_map>
#include <string>

namespace lazy_bedrock_breaker {
extern std::unordered_map<std::string, BlockPos>
    playerClickedPositions; // Map to store clicked block positions for each player.

extern std::string pistonFacingDirection[6];

void clearMap();

void onPlayerPlacingBlock(ll::event::PlayerPlacingBlockEvent& event);

void afterPlayerPlacedBlock(ll::event::PlayerPlacedBlockEvent& event);

void removeRedStoneItem(Player& player, unsigned char redstoneSlot);

void getReadyToBreak(BlockSource& blockSource, const BlockPos& pos, const Block& piston, BlockPos targetPos);

void clearAllRedStoneNearby(BlockSource& blockSource, const BlockPos& pos, bool isDrop);

bool findRedStoneBlock(BlockSource& blockSource, const BlockPos& pos, unsigned char facing);

bool getRedStoneBlockReady(BlockSource& blockSource, const BlockPos& pos, unsigned char facing);

std::optional<unsigned char> getPistonReady(BlockSource& blockSource, const BlockPos& pos, const Block& piston);

std::optional<unsigned char> searchAvilablePistonFacingDirection(BlockSource& blockSource, const BlockPos& pos);

std::optional<unsigned char> getRedStoneFromInventory(const Player& player);

unsigned char handleDirection(unsigned char direction);
}