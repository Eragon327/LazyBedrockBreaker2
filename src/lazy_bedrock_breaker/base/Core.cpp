#include "Core.h"
#include "Mod.h"
#include "Task.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/CompoundTagVariant.h"
#include "mc/nbt/IntTag.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
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
    for (unsigned char i = 0; i < 6; i++) {
        if (blockSource.getBlock(pos.relative(i, 1)).isAir()) {
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
        unsigned char direction = states["facing_direction"].get<IntTag>();
        direction = handleDirection(direction);
        if (blockSource.getBlock(pos.relative(direction, 1)).isAir()) return std::make_optional(direction);  // 无需旋转

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
    for (unsigned char i = 0; i < 6; i++) {
        if(i == facing) continue; // 跳过活塞朝向的那个位置
        if (blockSource.getBlock(pos.relative(i, 1)).getTypeName() == "minecraft:redstone_block") {
            return true; // Found a redstone block in the adjacent positions
        }
    }
    return false; // No redstone block found
}

bool getRedStoneBlockReady(BlockSource& blockSource, const BlockPos& pos, unsigned char facing) {
    for (unsigned char i = 0; i < 6; i++) {
        if(i == facing) continue; // 跳过活塞朝向的那个位置
        BlockPos adjacentPos = pos.relative(i, 1);
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
    for (unsigned char i = 0; i < 6; i++) {
        BlockPos adjacentPos = pos.relative(i, 1);
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

    BlockPos targetPos = playerClickedPositions[uuid.asString()];
    TaskManager::addTask(
        [&blockSource, pos, &block, targetPos, isCreative]() {
            getReadyToBreak(blockSource, pos, block, targetPos);
            clearAllRedStoneNearby(blockSource, pos, !isCreative);

            // 这里我们嵌套一个Task
            TaskManager::addTask(
                [&blockSource, pos, isCreative]() {
                    ll::service::getLevel()->destroyBlock(blockSource, pos, !isCreative); // 破坏活塞本身
                },
                4
            );
        },
        3
    );
    // 目前这些数值都是手调的
    // TODO：做检测式，而不是单纯的延时，确保在活塞完成旋转后再执行后续逻辑
}

}