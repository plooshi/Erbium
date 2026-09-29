#pragma once
#include "../../pch.h"
#include "BotState.h"

class FBotController;

// One AI state = one behavior object.
//
// FBotController owns exactly one active behavior at a time and switches between them through
// FBotController::SetAIState, which guarantees OnExit(old) -> OnEnter(new) ordering. Behaviors never
// switch themselves directly; they request a transition via Bot.SetAIState(...) from Tick.
//
// Tick is called at FBotSystemConfig::ThinkInterval (not every frame). DeltaSeconds is the real time
// since this bot's previous think.
class IBotBehavior
{
public:
    virtual ~IBotBehavior() = default;

    virtual EBotAIState GetState() const = 0;

    virtual void OnEnter(FBotController& Bot) {}
    virtual void Tick(FBotController& Bot, float DeltaSeconds) {}
    virtual void OnExit(FBotController& Bot) {}
};

// Phase 1: the bot stands still and keeps its pawn alive. Exists so the controller always has a valid
// behavior and so the enter/exit plumbing is exercised before real behaviors are added.
class FBotIdleBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Idle; }
};

// Terminal behavior while the bot is dead and waiting for cleanup.
class FBotDeadBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Dead; }
};

// Creates the behavior object for a state. Returns nullptr for EBotAIState::None.
std::unique_ptr<IBotBehavior> CreateBotBehavior(EBotAIState State);
