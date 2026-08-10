#pragma once

#include "Config.h"
#include "ll/api/event/ListenerBase.h"
#include "ll/api/mod/NativeMod.h"
#include <memory>


namespace lazy_bedrock_breaker {

class LazyBedrockBreaker {
    struct Impl;
    std::unique_ptr<Impl> mImpl;

public:
    LazyBedrockBreaker();
    ~LazyBedrockBreaker();

    static LazyBedrockBreaker& getInstance();

    [[nodiscard]] config::Config& getConfig();
    [[nodiscard]] std::set<ll::event::ListenerPtr>& getEventListener();

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

    /// @return True if the mod is unloaded successfully.
    bool unload();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace lazy_bedrock_breaker
