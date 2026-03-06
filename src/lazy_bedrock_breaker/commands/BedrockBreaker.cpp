#include "Commands.h"
#include "../base/Mod.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/runtime/ParamKind.h"
#include "ll/api/command/runtime/RuntimeCommand.h"
#include "ll/api/command/runtime/RuntimeOverload.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/world/actor/player/Player.h"
#include <string>

namespace lazy_bedrock_breaker::commands {
void registerBedrockBreakerCommand() {

    // reg cmd
    ll::command::CommandHandle& bedrockBreakerCommand = ll::command::CommandRegistrar::getInstance().getOrCreateCommand(
        "bb",
        "懒狗破基岩个人设置",
        CommandPermissionLevel::Any
    );

    // bb isopen <bool>
    bedrockBreakerCommand.runtimeOverload()
        .text("isopen")
        .required("enabled", ll::command::ParamKind::Bool)
        .execute([](CommandOrigin const& origin, CommandOutput& output, ll::command::RuntimeCommand const& self) {
            Actor* entity = origin.getEntity();
            if (entity == nullptr || !entity->isType(ActorType::Player)) {
                output.error("只有玩家才能使用这个命令");
                return;
            }
            Player*          player = static_cast<Player*>(entity);
            const mce::UUID& uuid   = player->getUuid();

            bool enabled = self["enabled"].get<ll::command::ParamKind::Bool>();
            if (enabled) {
                if (!lazy_bedrock_breaker::mod().getPlayerDb()->get("function_enabled")) {
                    output.error("管理员尚未开启全局功能，无法启用个人设置");
                    return;
                }
                lazy_bedrock_breaker::mod().getPlayerDb()->set(uuid.asString(), "true");
            } else {
                lazy_bedrock_breaker::mod().getPlayerDb()->del(uuid.asString());
            }

            output.success("已将{}懒狗破基岩!", enabled ? "开启" : "关闭");
            return;
        });
    

    // bb list
    bedrockBreakerCommand.runtimeOverload().text("list").execute(
        [](CommandOrigin const& origin, CommandOutput& output, ll::command::RuntimeCommand const&) {
            Actor* entity = origin.getEntity();
            if(entity == nullptr) {
                output.error("无效的命令来源");
                return;
            }
            std::vector<std::string> blocks = lazy_bedrock_breaker::arr().getAll();
            std::string blockList;
            for (const std::basic_string<char>& block : blocks) {
                blockList += "\n" + block;
            }
            if (blockList.empty()) {
                output.success("当前懒狗破基岩列表为空");
                return;
            }
            output.success("当前懒狗破基岩列表:{}", blockList);
            return;
        }
    );
}
}