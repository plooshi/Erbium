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
//   Idle/Searching/Moving/Following ──> Building (hit by a visible enemy / stuck / command)
//   Building ──> the state it interrupted
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

// Phase 4. Carries out FBotBuilder::Pending, then returns to Pending.ReturnState:
//   Reacting   - human-like delay before placing (BuildReactionTime, randomised)
//   Placing    - FBotBuilder::Build: select piece, find/validate location, place, verify
//   Holding    - defensive wall: stay behind it for DefensiveHoldTime
//   Traversing - obstacle ramp: walk over it towards the goal that was blocked
// A failed placement returns immediately; the builder's cooldown prevents retry loops.
class FBotBuildingBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Building; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
    void OnExit(FBotController& Bot) override;

private:
    enum class EPhase : uint8
    {
        Reacting,
        Holding,
        Traversing,
    };

    void Finish(FBotController& Bot);

    EPhase Phase = EPhase::Reacting;
    float Timer = 0.f;
};

// Terminal state while the bot is dead and waiting for cleanup.
class FBotDeadBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Dead; }
    void OnEnter(FBotController& Bot) override;
};
