#pragma once
#include "../../pch.h"
#include "BotState.h"

class FBotController;

// One AI state = one behavior object.
//
// FBotController owns exactly one active behavior at a time and switches between them through
// FBotController::SetAIState, which guarantees OnExit(old) -> OnEnter(new) ordering. Behaviors request
// transitions by calling Bot.SetAIState(...) from Tick; after that call the behavior object has been
// destroyed, so Tick must return immediately.
//
// Tick is called at FBotSystemConfig::ThinkInterval (not every frame), after perception has updated
// and before movement executes. DeltaSeconds is the real time since this bot's previous think.
//
// Concrete behaviors live in Behaviors/BotBehaviors.h.
class IBotBehavior
{
public:
    virtual ~IBotBehavior() = default;

    virtual EBotAIState GetState() const = 0;

    virtual void OnEnter(FBotController& Bot) {}
    virtual void Tick(FBotController& Bot, float DeltaSeconds) {}
    virtual void OnExit(FBotController& Bot) {}
};

// Creates the behavior object for a state. Returns nullptr for EBotAIState::None.
std::unique_ptr<IBotBehavior> CreateBotBehavior(EBotAIState State);
