#pragma once
#include "../../pch.h"

// Lifecycle of a bot as seen by the manager. This is independent from the AI state below: the
// lifecycle says whether the bot exists in the world, the AI state says what it is trying to do.
enum class EBotLifeState : uint8
{
    Spawning,       // Actors are being created; not yet registered as active.
    Active,         // Possessing a live pawn.
    Dead,           // Pawn died or was destroyed; waiting for cleanup.
    PendingRemoval, // Scheduled for destruction on the next maintenance pass.
};

// Which engine controller type drives the bot. See BotCompat::GetBotControllerClass.
enum class EBotControllerKind : uint8
{
    // Client-less APlayerController (Phase 1). Can exist in the match but cannot move on a dedicated
    // server, because the movement component only simulates player pawns from client RPCs.
    PlayerController,

    // AAIController subclass. Its pawn is simulated on the server, so it can move and act.
    AIController,
};

// High-level AI state. Each value maps to one IBotBehavior (see Behaviors/BotBehaviors.h).
// Phase 2A: movement states. Looting/combat/healing states are added with their systems.
enum class EBotAIState : uint8
{
    None,
    Idle,      // Stand still.
    Searching, // Wander around the area looking for something to do.
    Moving,    // Go to a commanded location, then Idle.
    Following, // Stay close to a commanded actor.
    Building,  // Place a piece (defensive wall / obstacle ramp / commanded), then return (Phase 4).
    Dead,
};

inline const char* ToString(EBotLifeState State)
{
    switch (State)
    {
    case EBotLifeState::Spawning:
        return "Spawning";
    case EBotLifeState::Active:
        return "Active";
    case EBotLifeState::Dead:
        return "Dead";
    case EBotLifeState::PendingRemoval:
        return "PendingRemoval";
    }
    return "Unknown";
}

inline const char* ToString(EBotControllerKind Kind)
{
    return Kind == EBotControllerKind::AIController ? "AIController" : "PlayerController";
}

inline const char* ToString(EBotAIState State)
{
    switch (State)
    {
    case EBotAIState::None:
        return "None";
    case EBotAIState::Idle:
        return "Idle";
    case EBotAIState::Searching:
        return "Searching";
    case EBotAIState::Moving:
        return "Moving";
    case EBotAIState::Following:
        return "Following";
    case EBotAIState::Building:
        return "Building";
    case EBotAIState::Dead:
        return "Dead";
    }
    return "Unknown";
}
