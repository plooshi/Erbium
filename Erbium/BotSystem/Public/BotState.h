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

// High-level AI state. Phase 1 only uses Idle and Dead; later phases add Searching, Moving, Looting,
// Targeting, ... as separate IBotBehavior implementations.
enum class EBotAIState : uint8
{
    None,
    Idle,
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

inline const char* ToString(EBotAIState State)
{
    switch (State)
    {
    case EBotAIState::None:
        return "None";
    case EBotAIState::Idle:
        return "Idle";
    case EBotAIState::Dead:
        return "Dead";
    }
    return "Unknown";
}
