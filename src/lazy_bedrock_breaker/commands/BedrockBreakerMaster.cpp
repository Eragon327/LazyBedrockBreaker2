#include "Commands.h"
#include "../base/Mod.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/runtime/ParamKind.h"
#include "ll/api/command/runtime/RuntimeCommand.h"
#include "ll/api/command/runtime/RuntimeOverload.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/phys/HitResult.h"
#include "mc/world/phys/HitResultType.h"
#include <string>

namespace lazy_bedrock_breaker::commands {
void registerBedrockBreakerMasterCommand() {

    // reg cmd
    ll::command::CommandHandle& bedrockBreakerMasterCommand = ll::command::CommandRegistrar::getInstance().getOrCreateCommand(
        "bbmaster",
        "懒狗破基岩管理员设置",
        CommandPermissionLevel::GameDirectors
    );

    // bbmaster isopen <bool>
    bedrockBreakerMasterCommand.runtimeOverload()
        .text("isopen")
        .required("enabled", ll::command::ParamKind::Bool)
        .execute(
        [](CommandOrigin const&, CommandOutput& output, ll::command::RuntimeCommand const& self) {
            bool enabled = self["enabled"].get<ll::command::ParamKind::Bool>();
            if (enabled) {
                lazy_bedrock_breaker::mod().getPlayerDb()->set("function_enabled", "true");
            } else {
                lazy_bedrock_breaker::mod().getPlayerDb()->del("function_enabled");
            }

            output.success("懒狗破基岩全局功能已{}!", enabled ? "开启" : "关闭");
            return;
        }
    );

    // bbmaster add
    bedrockBreakerMasterCommand.runtimeOverload().text("add").execute(
        [](CommandOrigin const& origin, CommandOutput& output, ll::command::RuntimeCommand const&) {
            Actor* entity = origin.getEntity();
            if (entity == nullptr || !entity->isType(ActorType::Player)) {
                output.error("只有玩家才能使用这个命令");
                return;
            }

            HitResult res =
                entity->traceRay(5.25f, false, true, [](BlockSource const&, Block const&, bool) { return true; });
            if (res.mType == HitResultType::NoHit) {
                output.error("请瞄准一个方块");
                return;
            }

            BlockPos     blockPos = res.mBlock;
            Block const& block    = entity->getDimensionBlockSource().getBlock(blockPos);
            std::string  typeName = block.getTypeName();

            ArrayManager& arrManager = lazy_bedrock_breaker::arr();
            if (!arrManager.has(typeName)) {
                arrManager.add(typeName);
                if (!arrManager.saveToDB()) {
                    output.error("无法将数据保存到数据库");
                }
            }
            output.success("已将 {} 添加到破基岩列表!", typeName);
        }
    );

    // bbmaster remove
    bedrockBreakerMasterCommand.runtimeOverload().text("remove").execute(
        [](CommandOrigin const& origin, CommandOutput& output, ll::command::RuntimeCommand const&) {
            Actor* entity = origin.getEntity();
            if (entity == nullptr || !entity->isType(ActorType::Player)) {
                output.error("只有玩家才能使用这个命令");
                return;
            }

            HitResult res =
                entity->traceRay(5.25f, false, true, [](BlockSource const&, Block const&, bool) { return true; });
            if (res.mType == HitResultType::NoHit) {
                output.error("请瞄准一个方块");
                return;
            }

            BlockPos     blockPos = res.mBlock;
            Block const& block    = entity->getDimensionBlockSource().getBlock(blockPos);
            std::string  typeName = block.getTypeName();

            ArrayManager& arrManager = lazy_bedrock_breaker::arr();
            if (arrManager.has(typeName)) {
                arrManager.del(typeName);
                if (!arrManager.saveToDB()) {
                    output.error("无法将数据保存到数据库");
                }
                output.success("已将 {} 从破基岩列表中移除!", typeName);
            } else {
                output.error("{} 不在破基岩列表中!", typeName);
            }
        }
    );

    // bbmaster reset
    bedrockBreakerMasterCommand.runtimeOverload().text("reset").execute(
        [](CommandOrigin const& origin, CommandOutput& output, ll::command::RuntimeCommand const&) {
            Actor* entity = origin.getEntity();
            if (entity == nullptr || !entity->isType(ActorType::Player)) {
                output.error("只有玩家才能使用这个命令");
                return;
            }

            ArrayManager& arrManager = lazy_bedrock_breaker::arr();
            arrManager               = ArrayManager();
            arrManager.add("minecraft:bedrock");

            if (!arrManager.saveToDB()) {
                output.error("无法将数据保存到数据库");
                return;
            }
            output.success("已重置破基岩列表!");
        }
    );

    // bbmaster list
    bedrockBreakerMasterCommand.runtimeOverload().text("list").execute(
        [](CommandOrigin const& origin, CommandOutput& output, ll::command::RuntimeCommand const&) {
            Actor* entity = origin.getEntity();
            if (entity == nullptr) {
                output.error("无效的命令来源");
                return;
            }
            std::vector<std::string> blocks = lazy_bedrock_breaker::arr().getAll();
            std::string blockList;
            for (const std::basic_string<char>& block : blocks) {
                blockList += "\n" + block;
            }
            if (blockList.empty()) {
                output.success("当前破基岩列表为空");
                return;
            }
            output.success("当前破基岩列表:{}", blockList);
        }
    );
}
}