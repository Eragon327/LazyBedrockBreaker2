#include "Core.h"
#include "Mod.h"
#include "Task.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/CompoundTagVariant.h"
#include "mc/nbt/IntTag.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionHeightRange.h"
#include "mc/world/level/GameType.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/player/PlayerInventory.h"
#include "mc/world/actor/player/Inventory.h"
#include "mc/world/item/ItemStackBase.h"
#include "ll/api/service/Bedrock.h"

#include "ll/api/io/LoggerRegistry.h"
#include <memory>
#include <optional>
#include <vector>
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

std::optional<unsigned char> searchAvilablePistonFacingDirection(BlockSource& blockSource, const BlockPos& pos) {
    DimensionHeightRange heightRange = blockSource.getDimension().mHeightRange;
    for (unsigned char i = 0; i < 6; i++) {
        BlockPos adjacentPos = pos.relative(i, 1);
        if (adjacentPos.y < heightRange.mMin || adjacentPos.y >= heightRange.mMax) continue; // 超出世界高度范围
        if (blockSource.getBlock(adjacentPos).isAir()) {
            return i; // Return the first available direction
        }
    }
    return std::nullopt; // Return std::nullopt if no available direction is found
}

std::optional<unsigned char> getPistonReady(BlockSource& blockSource, const BlockPos& pos, const Block& piston) {
    // 必须保证这个方块是活塞, 才能获取正确的nbtTag, 以便后续旋转使用
    CompoundTag nbtTag = *piston.mSerializationId;
    if (!nbtTag.contains("states")) return std::nullopt;
    CompoundTagVariant states = nbtTag["states"];
    if (states.contains("facing_direction")) {
        DimensionHeightRange heightRange = blockSource.getDimension().mHeightRange;
        unsigned char        direction   = states["facing_direction"].get<IntTag>();
        direction                        = handleDirection(direction);
        BlockPos facingPos               = pos.relative(direction, 1);
        if (facingPos.y > heightRange.mMin && facingPos.y < heightRange.mMax && blockSource.getBlock(facingPos).isAir())
            return std::make_optional(direction);
        // 无需旋转

        std::optional<unsigned char> newDirection = searchAvilablePistonFacingDirection(blockSource, pos);
        if(!newDirection) return std::nullopt; // 没有可用的方向
        unsigned char newDirectionValue = handleDirection(newDirection.value());
        CompoundTag newTag = nbtTag;
        newTag["states"]["facing_direction"] = IntTag(newDirectionValue);
        optional_ref<const Block> newBlock   = Block ::tryGetFromRegistry(newTag);
        if (!newBlock) return std::nullopt;

        blockSource.setBlock(pos, *newBlock, 3, nullptr, nullptr);
        return std::make_optional(newDirection.value());
    }
    return std::nullopt;
}

std::optional<unsigned char> getRedStoneFromInventory(const Player& player) {
    std::vector<ItemStack> items = player.mInventory->mInventory->mItems;
    for (int i = 0; i < items.size(); i++) {
        if(items[i].getTypeName() == "minecraft:redstone_block") {
            return std::make_optional(static_cast<unsigned char>(i)); // Return the slot index of the redstone block
        }
    }
    return std::nullopt; // Return std::nullopt if no redstone block is found
}

bool findRedStoneBlock(BlockSource& blockSource, const BlockPos& pos, unsigned char facing) {
    DimensionHeightRange heightRange = blockSource.getDimension().mHeightRange;
    for (unsigned char i = 0; i < 6; i++) {
        if(i == facing) continue; // 跳过活塞朝向的那个位置
        BlockPos adjacentPos = pos.relative(i, 1);
        if (adjacentPos.y < heightRange.mMin || adjacentPos.y >= heightRange.mMax) continue; // 超出世界高度范围
        if (blockSource.getBlock(adjacentPos).getTypeName() == "minecraft:redstone_block") {
            return true; // Found a redstone block in the adjacent positions
        }
    }
    return false; // No redstone block found
}

bool getRedStoneBlockReady(BlockSource& blockSource, const BlockPos& pos, unsigned char facing) {
    DimensionHeightRange heightRange = blockSource.getDimension().mHeightRange;
    for (unsigned char i = 0; i < 6; i++) {
        if(i == facing) continue; // 跳过活塞朝向的那个位置
        BlockPos adjacentPos = pos.relative(i, 1);
        if (adjacentPos.y < heightRange.mMin || adjacentPos.y >= heightRange.mMax) continue; // 超出世界高度范围
        if (blockSource.getBlock(adjacentPos).getTypeName() == "minecraft:air") {
            if (blockSource.setBlock(
                    adjacentPos,
                    Block::tryGetFromRegistry("minecraft:redstone_block").value(),
                    3,
                    nullptr,
                    nullptr
                ))
                return true;          
        }
    }
    return false; // No redstone block found
}

void removeRedStoneItem(Player& player, unsigned char redstoneSlot) {
    player.mInventory->mInventory->removeItem(redstoneSlot, 1);
}

void getReadyToBreak(BlockSource& blockSource, const BlockPos& pos, const Block& piston, BlockPos targetPos) {
    unsigned char newDirectionValue = 0;
    for (newDirectionValue = 0; newDirectionValue < 6; newDirectionValue++) {
        if (pos.relative(newDirectionValue, 1) == targetPos) break;
    }

    CompoundTag nbtTag = *piston.mSerializationId;
    CompoundTag newTag = nbtTag;
    newTag["states"]["facing_direction"] = IntTag(handleDirection(newDirectionValue));
    optional_ref<const Block> newBlock   = Block ::tryGetFromRegistry(newTag);
    if (newBlock) blockSource.setBlock(pos, *newBlock, 3, nullptr, nullptr);
}

void clearAllRedStoneNearby(BlockSource& blockSource, const BlockPos& pos, bool isDrop) {
    DimensionHeightRange heightRange = blockSource.getDimension().mHeightRange;
    for (unsigned char i = 0; i < 6; i++) {
        BlockPos adjacentPos = pos.relative(i, 1);
        if (adjacentPos.y < heightRange.mMin || adjacentPos.y >= heightRange.mMax) continue; // 超出世界高度范围
        if (blockSource.getBlock(adjacentPos).getTypeName() == "minecraft:redstone_block")
            ll::service::getLevel()->destroyBlock(blockSource, adjacentPos, isDrop); // 直接破坏方块，触发掉落逻辑
    }
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

    std::optional<unsigned char> facing = getPistonReady(blockSource, pos, block);
    if (!facing) return;

    bool isCreative = player.getPlayerGameType() == GameType::Creative;

    bool redstoneNearby = findRedStoneBlock(blockSource, pos, facing.value());
    std::optional<unsigned char> redstoneSlot = isCreative ? std::nullopt : getRedStoneFromInventory(player);
    if (!redstoneNearby && !isCreative && !redstoneSlot) return; // 玩家没有红石块

    if (!redstoneNearby) {  // 按照逻辑，如果附近没有红石块，那么redstoneslot不可能为空，不然到不了这里
        if (getRedStoneBlockReady(blockSource, pos, facing.value())) {
            if (!isCreative) removeRedStoneItem(player, redstoneSlot.value());
        }
        else return; // 无法放置红石块
    }

    BlockPos facingPos = pos.relative(facing.value(), 1);
    BlockPos targetPos = playerClickedPositions[uuid.asString()];
    auto waitTicks = std::make_shared<int>(0);
    const int MAX_WAIT_TICKS = 10;

    TaskManager::addTask(
        [&blockSource, facingPos, targetPos, pos, facing, waitTicks, &player, isCreative]() mutable {
            std::string typeName = blockSource.getBlock(facingPos).getTypeName();
            bool        hasPushed =
                typeName == "minecraft:piston_arm_collision" || typeName == "minecraft:sticky_piston_arm_collision";

            if (hasPushed) {
                getReadyToBreak(blockSource, pos, blockSource.getBlock(pos), targetPos);
                clearAllRedStoneNearby(blockSource, pos, false);

                // 添加收回任务 (RetractTask)
                TaskManager::addTask([&blockSource, targetPos, pos, isCreative]() {
                    bool retracted = blockSource.getBlock(targetPos).isAir(); // 活塞已经收回的标志是原位置变成了空气
                    if (retracted) {
                        ll::service::getLevel()->destroyBlock(blockSource, pos, !isCreative);
                    }
                    return retracted; // 任务完成条件：活塞已经收回
                });
                return true;
            }

            // 超时处理
            (*waitTicks)++;
            if (*waitTicks >= MAX_WAIT_TICKS) {
                std::optional<unsigned char> redstoneSlot = isCreative ? std::nullopt : getRedStoneFromInventory(player);
                if (!isCreative && !redstoneSlot) {
                    // 彻底失败：没有红石补给
                    clearAllRedStoneNearby(blockSource, pos, true);
                    ll::service::getLevel()->destroyBlock(blockSource, pos, true);
                    return true; 
                } else {
                    // 尝试再次放置红石块
                    if (getRedStoneBlockReady(blockSource, pos, facing.value())) {
                        if (!isCreative) removeRedStoneItem(player, redstoneSlot.value());
                        *waitTicks = 0; // 重置计数器，再给机会
                        return false;
                    } else {
                        // 无法放置红石块，终止
                        clearAllRedStoneNearby(blockSource, pos, true);
                        ll::service::getLevel()->destroyBlock(blockSource, pos, true);
                        return true;
                    }
                }
            }

            return false;
        }
    );
}

}