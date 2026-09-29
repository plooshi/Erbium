#include "pch.h"
#include "../Public/BotBehavior.h"
#include "../Public/Behaviors/BotBehaviors.h"

std::unique_ptr<IBotBehavior> CreateBotBehavior(EBotAIState State)
{
    switch (State)
    {
    case EBotAIState::Idle:
        return std::make_unique<FBotIdleBehavior>();
    case EBotAIState::Searching:
        return std::make_unique<FBotSearchingBehavior>();
    case EBotAIState::Moving:
        return std::make_unique<FBotMovingBehavior>();
    case EBotAIState::Following:
        return std::make_unique<FBotFollowingBehavior>();
    case EBotAIState::Building:
        return std::make_unique<FBotBuildingBehavior>();
    case EBotAIState::Dead:
        return std::make_unique<FBotDeadBehavior>();
    case EBotAIState::None:
        break;
    }

    return nullptr;
}
