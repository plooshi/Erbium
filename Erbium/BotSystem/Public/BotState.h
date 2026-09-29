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
// One behavior per state (Behaviors/BotBehaviors.h). Searching/Looting/Combat/Healing/Escaping are
// chosen by the brain (Decision/BotBrain.h); Idle/Moving/Following are commanded; Building interrupts
// and returns.
enum class EBotAIState : uint8
{
    None,
    Idle,      // Stand still.
    Searching, // Wander around the area looking for something to do.
    Moving,    // Go to a commanded location, then Idle.
    Following, // Stay close to a commanded actor.
    Building,  // Place a piece (defensive wall / obstacle ramp / commanded), then return (Phase 4).
    Looting,   // Walk to a pickup and take it (Phase 2B).
    Combat,    // Target, aim, shoot, reload, reposition (Phase 2C).
    Healing,   // Use a healing / shield item (Phase 2C).
    Escaping,  // Retreat from a threat (Phase 2C).
    Rotating,  // Move into the next storm circle (Phase 6).
    InBus,     // Riding the battle bus, waiting for its jump time (Phase 6).
    Skydiving, // Falling/gliding towards a landing spot (Phase 6).
    Dead,
};

// Phase 5.1: high-level priorities.
enum class EBotPriority : uint8
{
    Search,   // -> Searching
    Loot,     // -> Looting
    Combat,   // -> Combat
    Healing,  // -> Healing
    Survival, // -> Escaping
    Storm,    // -> Rotating
    Count,
};

// Phase 5.2: difficulty presets. They only change reaction/accuracy/tactics parameters, never what the
// bot is allowed to know.
enum class EBotDifficulty : uint8
{
    Easy,
    Normal,
    Hard,
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
    case EBotAIState::Looting:
        return "Looting";
    case EBotAIState::Combat:
        return "Combat";
    case EBotAIState::Healing:
        return "Healing";
    case EBotAIState::Escaping:
        return "Escaping";
    case EBotAIState::Rotating:
        return "Rotating";
    case EBotAIState::InBus:
        return "InBus";
    case EBotAIState::Skydiving:
        return "Skydiving";
    case EBotAIState::Dead:
        return "Dead";
    }
    return "Unknown";
}
