#pragma once
#include "../BotBehavior.h"
#include "../../../FortniteGame/Public/FortInventory.h"

// Behaviors: standing, wandering, commanded movement (2A), looting (2B), combat / healing / escaping
// (2C), building (4). The brain (Decision/BotBrain.h) chooses between the autonomous ones.
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
//   Searching <-> Looting / Combat / Healing / Escaping   (chosen by FBotBrain by score)

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

// Phase 2B. Walk to the pickup the brain chose (claimed so other bots leave it), take it, re-equip.
// Gives up (and ignores that pickup for a while) when it cannot be reached.
class FBotLootingBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Looting; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
    void OnExit(FBotController& Bot) override;

private:
    TWeakObjectPtr<AFortPickupAthena> Pickup;
    TWeakObjectPtr<AActor> Container; // chest / ammo box (Phase 6) instead of a pickup
    bool bIsContainer = false;
    float TimeLeft = 0.f;
    float SearchTimeLeft = -1.f;      // >= 0 while standing at the container
};

// Phase 2C. Nested combat state machine driven by FBotCombat (targeting, aim, fire, reload) plus
// positioning here:
//   Approach   - target further than the weapon's preferred range: move closer
//   Engage     - in range: stand and shoot, strafe to a new spot every RepositionInterval
//   Back off   - target too close for the weapon: step away
//   Search     - target out of sight (within TargetMemory): go to where it was last seen
class FBotCombatBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Combat; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
    void OnExit(FBotController& Bot) override;

private:
    double NextRepositionTime = 0.0;
    double NextMoveDecisionTime = 0.0;
};

// Phase 2C. Use the most fitting healing/shield item: equip it and hold "fire" (how consumables are
// used), until the item count drops or it times out. Virtual-inventory bots emulate the effect (see
// FBotSystemConfig::bEmulateConsumablesForVirtualInventory).
class FBotHealingBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Healing; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
    void OnExit(FBotController& Bot) override;

private:
    FGuid ItemGuid{};
    const UFortItemDefinition* ItemDefinition = nullptr;
    int CountBefore = 0;
    float TimeLeft = 0.f;
    float EmulatedUseTime = -1.f;
    bool bUsing = false;
};

// Phase 2C. Sprint away from the nearest threat, then let the brain decide again.
class FBotEscapingBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Escaping; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
    void OnExit(FBotController& Bot) override;

private:
    float TimeLeft = 0.f;
};

// Phase 6. Move into the next storm circle (sprinting when far), then let the brain decide again.
class FBotRotatingBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Rotating; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
    void OnExit(FBotController& Bot) override;

private:
    double NextRepathTime = 0.0;
};

// Phase 6. On the battle bus: the pawn is hidden and out of play until the bot's jump time inside the
// drop window, then it is placed at the bus and starts skydiving (what a player's jump does).
class FBotInBusBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::InBus; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
};

// Phase 6. Steer the fall/glide towards a landing spot (a chest when one is near), then Searching.
class FBotSkydivingBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Skydiving; }
    void OnEnter(FBotController& Bot) override;
    void Tick(FBotController& Bot, float DeltaSeconds) override;
    void OnExit(FBotController& Bot) override;

private:
    float TimeLeft = 0.f;
    float GroundedTime = 0.f;
};

// Terminal state while the bot is dead and waiting for cleanup.
class FBotDeadBehavior final : public IBotBehavior
{
public:
    EBotAIState GetState() const override { return EBotAIState::Dead; }
    void OnEnter(FBotController& Bot) override;
};
