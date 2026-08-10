#include "LazyBedrockBreaker.h"
#include "Core.h"
#include "ll/api/Config.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerPlaceBlockEvent.h"
#include "ll/api/mod/RegisterHelper.h"


namespace lazy_bedrock_breaker {
struct LazyBedrockBreaker::Impl {
    config::Config                   mConfig;
    std::set<ll::event::ListenerPtr> mEventListeners;
};

LazyBedrockBreaker::LazyBedrockBreaker() : mImpl(std::make_unique<Impl>()), mSelf(*ll::mod::NativeMod::current()) {}

LazyBedrockBreaker::~LazyBedrockBreaker() = default;

LazyBedrockBreaker& LazyBedrockBreaker::getInstance() {
    static LazyBedrockBreaker instance;
    return instance;
}

config::Config& LazyBedrockBreaker::getConfig() { return mImpl->mConfig; }

std::set<ll::event::ListenerPtr>& LazyBedrockBreaker::getEventListener() { return mImpl->mEventListeners; }

bool LazyBedrockBreaker::load() {
    const auto& logger = getSelf().getLogger();
    // load config
    const auto& configFilePath = getSelf().getConfigDir() / "config.json";
    auto&       config         = getConfig();
    if (!ll::config::loadConfig(config, configFilePath)) {
        logger.warn("Cannot load configurations from {}", configFilePath);
        logger.info("Saving default configurations");

        if (!ll::config::saveConfig(config, configFilePath)) {
            logger.error("Cannot save default configurations to {}", configFilePath);
        }
    }

    getEventListener().emplace(
        ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerPlacingBlockEvent>(
            [](ll::event::PlayerPlacingBlockEvent& event) { core::onPlayerPlacingBlock(event); }
        )
    );

    getEventListener().emplace(
        ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerPlacedBlockEvent>(
            [](ll::event::PlayerPlacedBlockEvent& event) { core::afterPlayerPlacedBlock(event); }
        )
    );

    return true;
}

bool LazyBedrockBreaker::enable() { return true; }

bool LazyBedrockBreaker::disable() { return true; }

bool LazyBedrockBreaker::unload() {
    getEventListener().clear();
    return true;
}

} // namespace lazy_bedrock_breaker

LL_REGISTER_MOD(lazy_bedrock_breaker::LazyBedrockBreaker, lazy_bedrock_breaker::LazyBedrockBreaker::getInstance());
