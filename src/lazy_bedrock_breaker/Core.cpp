#include "Core.h"
#include "LazyBedrockBreaker.h"
#include "ll/api/base/StdInt.h"
#include "mc/deps/nbt/IntTag.h"
#include "mc/network/packet/InventoryTransactionPacket.h"
#include "mc/network/packet/ItemStackRequestPacket.h"
#include "mc/world/actor/player/Inventory.h"
#include "mc/world/inventory/network/ItemStackRequestActionSwap.h"
#include "mc/world/inventory/network/ItemStackRequestData.h"
#include "mc/world/inventory/transaction/ItemUseInventoryTransaction.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/BlockType.h"

#include <algorithm>
#include <optional>
#include <vector>


namespace lazy_bedrock_breaker::core {

std::unordered_map<mce::UUID, BlockPos> playerClickedPositions;
// constexpr std::string                   facingDirection[6] = {"down", "up", "north", "south", "west", "east"};

uchar fixPistonFacing(uchar facing) {
    switch (facing) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 3;
    case 3:
        return 2;
    case 4:
        return 5;
    case 5:
        return 4;
    default:
        return facing;
    }
}

std::optional<uchar> getFacingDirection(const CompoundTag& nbtTag) {
    if (!nbtTag.contains("states")) {
        return std::nullopt;
    }
    const auto& states = nbtTag["states"];
    if (states.contains("facing_direction")) {
        uchar direction = static_cast<uchar>(states["facing_direction"].get<IntTag>());
        return fixPistonFacing(direction);
    }
    return std::nullopt;
}

bool hasRedstoneBlock(
    const BlockPos&           pistonPos,
    const BlockSource&        blockSource,
    const std::vector<uchar>& excludeDirections = {}
) {
    for (uchar direction = 0; direction < 6; ++direction) {
        if (std::find(excludeDirections.begin(), excludeDirections.end(), direction) != excludeDirections.end()) {
            continue;
        }
        const auto neighborPos = pistonPos.neighbor(direction);
        if (blockSource.getBlock(neighborPos).getTypeName() == "minecraft:redstone_block") {
            return true;
        }
    }
    return false;
}

std::optional<BlockPos> getEmptyNeighbor(
    const BlockPos&           pos,
    const BlockSource&        blockSource,
    const std::vector<uchar>& excludeDirections = {}
) {
    for (uchar direction = 0; direction < 6; ++direction) {
        if (std::find(excludeDirections.begin(), excludeDirections.end(), direction) != excludeDirections.end()) {
            continue;
        }
        const auto neighborPos = pos.neighbor(direction);
        if (blockSource.getBlock(neighborPos).isAir()) {
            return neighborPos;
        }
    }
    return std::nullopt;
}

std::optional<uchar> getNeighborFacing(const BlockPos& pos, const BlockPos& neighborPos) {
    for (uchar direction = 0; direction < 6; ++direction) {
        if (pos.neighbor(direction) == neighborPos) {
            return fixPistonFacing(direction);
        }
    }
    return std::nullopt;
}

std::optional<int> findFirstItemSlot(const Inventory& inventory, const std::string& blockTypeName) {
    for (int slot = 0; slot < inventory.getContainerSize(); ++slot) {
        const auto& item = inventory.getItem(slot);
        if (item == ItemStack::EMPTY_ITEM()) {
            continue;
        }
        if (const auto& itemBlockType = item.getBlockType().get()) {
            if (itemBlockType->getTypeName() == blockTypeName) {
                return slot;
            }
        }
    }
    return std::nullopt;
}

std::optional<int> searchBestTool(const Inventory& inventory, const Block& block) {
    auto  bestToolSlot = std::optional<int>{};
    float bestSpeed    = 0.0f;
    for (int i = 0; i < inventory.getContainerSize(); ++i) {
        const auto& item = inventory.getItem(i);
        if (item == ItemStack::EMPTY_ITEM()) {
            continue;
        }
        float speed = item.getDestroySpeed(block);
        if (speed > bestSpeed) {
            bestSpeed    = speed;
            bestToolSlot = i;
        }
    }
    return bestToolSlot;
}

int searchBestSlotInHotbar(const Inventory& inventory) {
    // 1. 优先选择空槽
    auto firstEmptySlot = inventory.getFirstEmptySlot();
    if (firstEmptySlot >= 0 && firstEmptySlot < 9) {
        return firstEmptySlot;
    }

    // 2. 避开工具和第一组活塞
    bool foundPiston = false;
    for (int slot = 0; slot < 9; ++slot) {
        const auto& item = inventory.getItem(slot);
        if (item == ItemStack::EMPTY_ITEM()) {
            continue;
        }
        if (item.getMaxDamage() > 0) {
            continue; // 避开工具
        }
        if (const auto& itemBlockType = item.getBlockType().get()) {
            if (itemBlockType->getTypeName().ends_with("piston") && !foundPiston) {
                foundPiston = true;
                continue; // 避开第一组活塞
            }
        }
        return slot; // 返回第一个符合条件的槽位
    }

    return 8; // 如果没有找到合适的槽位，返回最后一个槽位
}

void swapItemInContainer(const Player& player, int slot1, int slot2) {
    if (slot1 == slot2) return;

    const auto& inventory = player.getInventory();
    const auto& srcItem   = inventory.getItem(slot1);
    const auto& dstItem   = inventory.getItem(slot2);

    auto getContainerName = [](int slot) -> ContainerEnumName {
        return (slot <= 8) ? ContainerEnumName::HotbarContainer : ContainerEnumName::InventoryContainer;
    };

    ItemStackRequestSlotInfo srcSlotInfo;
    srcSlotInfo.mFullContainerName.mName      = getContainerName(slot1);
    srcSlotInfo.mFullContainerName.mDynamicId = std::nullopt;
    srcSlotInfo.mSlot                         = static_cast<uchar>(slot1);
    srcSlotInfo.mNetIdVariant                 = srcItem.mNetIdVariant;

    ItemStackRequestSlotInfo dstSlotInfo;
    dstSlotInfo.mFullContainerName.mName      = getContainerName(slot2);
    dstSlotInfo.mFullContainerName.mDynamicId = std::nullopt;
    dstSlotInfo.mSlot                         = static_cast<uchar>(slot2);
    dstSlotInfo.mNetIdVariant =
        dstItem == ItemStack::EMPTY_ITEM() ? ItemStackNetIdVariant{} : dstItem.mNetIdVariant.get();

    auto action      = std::make_unique<ItemStackRequestActionSwap>(srcSlotInfo, dstSlotInfo);
    auto requestData = std::make_unique<ItemStackRequestData>();
    requestData->mActions->push_back(std::move(action)); // addAction API已弃用

    auto batch = std::make_unique<ItemStackRequestBatch>();
    batch->mRequests->push_back(std::move(requestData)); // addRequest API已弃用

    ItemStackRequestPacket(std::move(batch)).sendToServer();
}

bool clientPlaceBlock(const Player& clientPlayer, const BlockPos& placePos, int slot, uchar face = 0) {
    if (slot < 0 || slot > 8) {
        return false; // 不在快捷栏上的放不出来
    }

    const auto& item = clientPlayer.getInventory().getItem(slot);

    auto placeTrans          = std::make_unique<ItemUseInventoryTransaction>();
    placeTrans->mActionType  = ItemUseInventoryTransaction::ActionType::Place;
    placeTrans->mTriggerType = ItemUseInventoryTransaction::TriggerType::PlayerInput;
    placeTrans->mPos         = placePos;
    placeTrans->mFace        = face;
    placeTrans->mSlot        = slot;
    placeTrans->setSelectedItem(item);
    placeTrans->mFromPos               = clientPlayer.getPosition();
    placeTrans->mClickPos              = placePos;
    placeTrans->mClientPredictedResult = ItemUseInventoryTransaction::PredictedResult::Success;
    placeTrans->mClientCooldownState   = ItemUseInventoryTransaction::ClientCooldownState::Off;
    InventoryTransactionPacket(std::move(placeTrans), true).sendToServer();

    return true;
}

void onPlayerPlacingBlock(ll::event::PlayerPlacingBlockEvent& event) {
    const auto& player = event.self();
    const auto& uuid   = player.getUuid();

    const auto& pos  = event.pos();
    const auto& type = player.getDimensionBlockSourceConst().getBlock(pos).getTypeName();

    const auto& whiteList = LazyBedrockBreaker::getInstance().getConfig().whiteList;

    if (std::find(whiteList.begin(), whiteList.end(), type) != whiteList.end()) {
        playerClickedPositions[uuid] = pos;
    }
}

/*  重要: 主功能  */
void afterPlayerPlacedBlock(ll::event::PlayerPlacedBlockEvent& event) {
    // const auto& logger = LazyBedrockBreaker::getInstance().getSelf().getLogger();

    if (!event.placedBlock().getTypeName().ends_with("piston")) {
        return;
    }
    const auto& pistonPos = event.pos();

    const auto& nbtTag    = *event.placedBlock().mSerializationId;
    const auto  facingOpt = getFacingDirection(nbtTag);
    if (!facingOpt.has_value()) {
        return;
    }
    const auto facing = facingOpt.value();

    const auto& player    = event.self();
    auto&       inventory = player.getInventory();
    const auto& uuid      = player.getUuid();
    if (playerClickedPositions.find(uuid) == playerClickedPositions.end()) {
        return;
    }
    const auto neighborFacingOpt = getNeighborFacing(pistonPos, playerClickedPositions[uuid]);
    if (!neighborFacingOpt.has_value()) {
        return;
    }
    playerClickedPositions.erase(uuid);

    const auto& blockSource = player.getDimensionBlockSourceConst();

    if (!hasRedstoneBlock(pistonPos, blockSource, {facing})) {
        const auto emptyNeighbor = getEmptyNeighbor(pistonPos, blockSource, {facing});
        if (!emptyNeighbor.has_value()) {
            return;
        }

        const auto slotOpt = findFirstItemSlot(inventory, "minecraft:redstone_block");
        if (!slotOpt.has_value() || slotOpt.value() < 0) {
            return;
        }
        int slot = slotOpt.value();

        if (slot > 8) {
            int bestSlot = searchBestSlotInHotbar(inventory);
            swapItemInContainer(player, slot, bestSlot);
            slot = bestSlot;
        }

        if (!clientPlaceBlock(player, *emptyNeighbor, slot, 0)) {
            return;
        }
    }

    auto bestToolSlotOpt = searchBestTool(inventory, event.placedBlock());
    if (bestToolSlotOpt.has_value()) {
        if (bestToolSlotOpt.value() > 8) {
            int bestSlot = searchBestSlotInHotbar(inventory);
            swapItemInContainer(player, bestToolSlotOpt.value(), bestSlot);
            bestToolSlotOpt = bestSlot;
        }
        // 让玩家手持最优工具
    }

    // 破坏活塞

    // 重新放置活塞，让活塞对准基岩

    // 破坏红石块，最优工具不变

    // 破坏活塞
}

} // namespace lazy_bedrock_breaker::core
