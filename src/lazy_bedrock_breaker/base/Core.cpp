#include "Core.h"
#include "Mod.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/CompoundTagVariant.h"
#include "mc/nbt/IntTag.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/actor/player/Player.h"

#include "ll/api/io/LoggerRegistry.h"
auto logger = ll::io::LoggerRegistry::getInstance().getOrCreate("Core");

namespace lazy_bedrock_breaker {

std::unordered_map<std::string, BlockPos> playerClickedPositions;
std::string pistonFacingDirection[6] = {"down", "up", "north", "south", "west", "east"};

void clearMap() { playerClickedPositions.clear(); }

unsigned char handleDirection(unsigned char direction) {
    switch (direction) {
        // 修复了活塞水平方向反转的神必 bug
        case 0: return 0; // 0 -> down
        case 1: return 1; // 1 -> up
        case 2: return 3; // 2 -> south
        case 3: return 2; // 3 -> north
        case 4: return 5; // 4 -> east
        case 5: return 4; // 5 -> west
        default: return direction; // Invalid direction, return as is.
    }
}

unsigned char searchAvilablePistonFacingDirection(BlockSource& blockSource, const BlockPos& pos) {
    for (unsigned char i = 0; i < 6; i++) {
        if (blockSource.getBlock(pos.relative(i, 1)).isAir()) {
            return i; // Return the first available direction
        }
    }
    return 255; // Return invalid value
}

bool getPistonReady(BlockSource& blockSource, const BlockPos& pos, const Block& piston) {
    // 必须保证这个方块是活塞, 才能获取正确的nbtTag, 以便后续旋转使用
    CompoundTag nbtTag = *piston.mSerializationId;
    if (!nbtTag.contains("states")) return false;
    CompoundTagVariant states = nbtTag["states"];
    if (states.contains("facing_direction")) {
        unsigned char direction = states["facing_direction"].get<IntTag>();
        direction = handleDirection(direction);
        logger->info("Piston facing direction: {}", direction);
        if (blockSource.getBlock(pos.relative(direction, 1)).isAir()) return true;  // 无需旋转

        unsigned char newDirectionValue = searchAvilablePistonFacingDirection(blockSource, pos);
        if(newDirectionValue == 255) return false; // 没有可用的方向
        newDirectionValue = handleDirection(newDirectionValue);
        CompoundTag newTag = nbtTag;
        newTag["states"]["facing_direction"] = IntTag(newDirectionValue);
        optional_ref<const Block> newBlock   = Block ::tryGetFromRegistry(newTag);
        if (!newBlock) return false;

        blockSource.setBlock(pos, *newBlock, 3, nullptr, nullptr);
        return true;
    }
    return false;
}

void onPlayerPlacingBlock(ll::event::PlayerPlacingBlockEvent& event) {
    if (!lazy_bedrock_breaker::mod().getPlayerDb()->get("function_enabled")) return;

    Player& player = event.self();
    const mce::UUID& uuid = player.getUuid();
    if(!lazy_bedrock_breaker::mod().getPlayerDb()->get(uuid.asString())) return;

    const BlockPos& pos = event.pos();
    const Block&    block = player.getDimensionBlockSource().getBlock(pos);
    std::string     typeName = block.getTypeName();

    if(lazy_bedrock_breaker::arr().has(typeName)) {
        playerClickedPositions[uuid.asString()] = pos;
    }
}

/*  重要: 主功能  */
void afterPlayerPlacedBlock(ll::event::PlayerPlacedBlockEvent& event) {
    if (!lazy_bedrock_breaker::mod().getPlayerDb()->get("function_enabled")) return;
    
    Player& player = event.self();
    const mce::UUID& uuid = player.getUuid();
    if (!lazy_bedrock_breaker::mod().getPlayerDb()->get(uuid.asString()) || !playerClickedPositions.count(uuid.asString())) return;

    BlockSource& blockSource = player.getDimensionBlockSource();
    const BlockPos& pos         = event.pos();
    const Block&    block       = blockSource.getBlock(pos);
    if (block.getTypeName() != "minecraft:piston" && block.getTypeName() != "minecraft:sticky_piston") return;

    if (!getPistonReady(blockSource, pos, block)) return;
}

}