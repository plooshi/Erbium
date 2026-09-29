#pragma once
#include "../BotBehavior.h"

// Phase 2A behaviors: standing, wandering, commanded movement.
//
// Transition overview:
//
//   spawn ──> DefaultAIState (Searching) ─┐
//                                         │ commands (cheats) from any state:
//   Idle <── Moving (arrived / failed)    ├──> Moving    (CommandMoveTo)
//   Searching <── Following (target lost) └──> Following (CommandFollow)
//   any ──> Dead (pawn died)
//
// Looting / Targeting / Attacking / Healing / Escaping arrive with their systems (Phase 2B/2C) and
// will be selected by Searching when perception reports something worth doing.

// Stand still.
class FBotIdleBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Idle; }
    void OnEnter(FBotController& Bot) override;
};

// Wander: pick a random (reachable, when there is a nav mesh) point within SearchRadius, walk there,
// pause for a moment, repeat.
class FBotSearchingBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Searching; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
    void OnExit(FBotController& Bot) override;

private:
    void PickNewDestination(FBotController& Bot);

    float PauseRemaining = 0.f;
};

// Walk to FBotController::GetCommandLocation(), then Idle.
class FBotMovingBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Moving; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
    void OnExit(FBotController& Bot) override;
};

// Stay within FollowDistance of FBotController::GetCommandTarget(); Searching when it is gone.
class FBotFollowingBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Following; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
    void OnExit(FBotController& Bot) override;
};

// Terminal state while the bot is dead and waiting for cleanup.
class FBotDeadBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Dead; }
    void OnEnter(FBotController& Bot) override;
};
