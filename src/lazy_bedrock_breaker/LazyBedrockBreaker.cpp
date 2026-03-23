#include "LazyBedrockBreaker.h"
#include "commands/Commands.h"
#include "base/Mod.h"
#include "base/Core.h"
#include "base/Task.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "mc/world/level/Level.h"

#include "mc/entity/systems/ActorLegacyTickSystem.h"


namespace lazy_bedrock_breaker {

namespace {
ll::event::ListenerPtr playerPlacingBlockEventListener;
ll::event::ListenerPtr playerPlacedBlockEventListener;
} // namespace

LazyBedrockBreaker& LazyBedrockBreaker::getInstance() {
    static LazyBedrockBreaker instance;
    return instance;
}

bool LazyBedrockBreaker::load() {
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
    commands::registerBedrockBreakerCommand();
    commands::registerBedrockBreakerMasterCommand();

    ll::event::EventBus& eventBus = ll::event::EventBus::getInstance();

    playerPlacingBlockEventListener =
        eventBus.emplaceListener<ll::event::PlayerPlacingBlockEvent>([](ll::event::PlayerPlacingBlockEvent &event) {
            onPlayerPlacingBlock(event);
        });

    playerPlacedBlockEventListener =
        eventBus.emplaceListener<ll::event::PlayerPlacedBlockEvent>([](ll::event::PlayerPlacedBlockEvent& event) {
            afterPlayerPlacedBlock(event);
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

LL_AUTO_TYPE_INSTANCE_HOOK(TickHook, HookPriority::Normal, Level, &Level::$tick, void) {
    origin();
    TaskManager::tick(); // TaskManager是静态类
    clearMap();
}

} // namespace lazy_bedrock_breaker

LL_REGISTER_MOD(lazy_bedrock_breaker::LazyBedrockBreaker, lazy_bedrock_breaker::LazyBedrockBreaker::getInstance());

