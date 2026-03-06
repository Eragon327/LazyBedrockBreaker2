#include "LazyBedrockBreaker.h"
#include "commands/Commands.h"
#include "base/Utils.h"
#include "base/Mod.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/BlockSource.h"
#include <unordered_map>


namespace lazy_bedrock_breaker {

namespace {
ll::event::ListenerPtr playerPlacingBlockEventListener;
ll::event::ListenerPtr playerPlacedBlockEventListener;
std::unordered_map<std::string, BlockPos> playerClickedPositions; // Map to store clicked block positions for each player.
}

LazyBedrockBreaker& LazyBedrockBreaker::getInstance() {
    static LazyBedrockBreaker instance;
    return instance;
}

bool LazyBedrockBreaker::load() {
    getSelf().getLogger().debug("Loading...");
    // Code for loading the mod goes here.

    // Initialize the player database.
    const std::filesystem::path& playerDbPath = getSelf().getDataDir() / "players";
    lazy_bedrock_breaker::mod().getPlayerDb() = std::make_unique<ll::data::KeyValueDB>(playerDbPath);

    // Initialize the block database.
    const std::filesystem::path& blockDbPath = getSelf().getDataDir() / "blocks";
    lazy_bedrock_breaker::mod().getBlockDb() = std::make_unique<ll::data::KeyValueDB>(blockDbPath);

    // Interact the block array with the block database.
    ArrayManager& arrManager = lazy_bedrock_breaker::arr();
    if (!arrManager.loadFromDB()) {
        getSelf().getLogger().error("Failed to load block array from database.");
        arrManager = ArrayManager(); // Reset to default state if loading fails.
    }
    
    return true;
}

bool LazyBedrockBreaker::enable() {
    getSelf().getLogger().debug("Enabling...");
    // Code for enabling the mod goes here.

    commands::registerBedrockBreakerCommand();
    commands::registerBedrockBreakerMasterCommand();

    ll::event::EventBus& eventBus = ll::event::EventBus::getInstance();

    playerPlacingBlockEventListener =
        eventBus.emplaceListener<ll::event::PlayerPlacingBlockEvent>([](ll::event::PlayerPlacingBlockEvent& event) {
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
    });

    playerPlacedBlockEventListener =
        eventBus.emplaceListener<ll::event::PlayerPlacedBlockEvent>([](ll::event::PlayerPlacedBlockEvent& event) {
            if (!lazy_bedrock_breaker::mod().getPlayerDb()->get("function_enabled")) return;
            
            Player& player = event.self();
            const mce::UUID& uuid = player.getUuid();
            if(!lazy_bedrock_breaker::mod().getPlayerDb()->get(uuid.asString())) return;


        });

    return true;
}

bool LazyBedrockBreaker::disable() {
    getSelf().getLogger().debug("Disabling...");
    // Code for disabling the mod goes here.

    ll::event::EventBus& eventBus = ll::event::EventBus::getInstance();

    eventBus.removeListener(playerPlacingBlockEventListener);
    eventBus.removeListener(playerPlacedBlockEventListener);

    return true;
}

bool LazyBedrockBreaker::unload() {
    getSelf().getLogger().debug("Unloading...");
    // Code for unloading the mod goes here.
    return true;
}

LL_AUTO_TYPE_INSTANCE_HOOK(TickHook, HookPriority::Normal, Level, &Level::$tick, void) {
    origin();
    playerClickedPositions.clear();
}

} // namespace lazy_bedrock_breaker

LL_REGISTER_MOD(lazy_bedrock_breaker::LazyBedrockBreaker, lazy_bedrock_breaker::LazyBedrockBreaker::getInstance());

