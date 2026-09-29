#include "pch.h"
#include "../Public/BotBehavior.h"

std::unique_ptr<IBotBehavior> CreateBotBehavior(EBotAIState State)
{
    switch (State)
    {
    case EBotAIState::Idle:
        return std::make_unique<FBotIdleBehavior>();
    case EBotAIState::Dead:
        return std::make_unique<FBotDeadBehavior>();
    case EBotAIState::None:
        break;
    }

    return nullptr;
}
