#pragma once
#include "../../../pch.h"

class FBotController;

// How a bot's pawn is moved. All modes use the character movement component (no teleporting); they
// differ in how the route is chosen.
enum class EBotNavMode : uint8
{
    None,        // Controller cannot drive its pawn (player-controller backend on a dedicated server).
    Pathfinding, // AAIController::MoveTo with nav-mesh pathfinding. Avoids obstacles.
    DirectPath,  // AAIController::MoveTo straight at the goal (no nav mesh needed, but needs a nav system).
    Steering,    // APawn::AddMovementInput towards the goal every frame. Works without any nav system.
};

enum class EBotNavRequestResult : uint8
{
    Failed,
    Moving,
    AlreadyAtGoal,
};

// Navigation strategy. Which strategies work depends on the build and the map (is there a navigation
// system? is nav mesh data loaded?), which cannot be known up front. So every strategy implements the
// same interface and BotNavigation picks the best one that has been seen to work, downgrading at
// runtime when one keeps failing.
//
// Implementations are stateless singletons shared by all bots; per-bot state (the current goal) lives
// in FBotMovement.
class IBotNavigation
{
public:
    virtual ~IBotNavigation() = default;

    virtual EBotNavMode GetMode() const = 0;

    virtual EBotNavRequestResult MoveToLocation(FBotController& Bot, const FVector& Goal, float AcceptanceRadius) = 0;
    virtual EBotNavRequestResult MoveToActor(FBotController& Bot, AActor* Goal, float AcceptanceRadius) = 0;
    virtual void Stop(FBotController& Bot) = 0;

    // Whether the underlying system is still executing the last request.
    virtual bool IsMoving(FBotController& Bot) const = 0;

    // Called every server frame while this mode is active (Steering needs continuous input).
    virtual bool NeedsFrameTick() const { return false; }
    virtual void TickFrame(FBotController& Bot, const FVector& Goal, float AcceptanceRadius) {}
};

namespace BotNavigation
{
    IBotNavigation* Get(EBotNavMode Mode);

    // Best mode believed to work on this server, for bots that can move at all.
    EBotNavMode GetPreferredMode();

    // Next mode to try after Mode failed (None when there is nothing left).
    EBotNavMode GetFallbackMode(EBotNavMode Mode);

    // Feeds request outcomes back. A mode that fails repeatedly without ever succeeding is dropped for
    // the rest of the match; one that has worked before is kept (individual goals can be unreachable).
    void ReportResult(EBotNavMode Mode, bool bSucceeded);

    // Random reachable point on the nav mesh, or a random point on the circle when there is no usable
    // nav data (the movement layer's stuck handling copes with unreachable points).
    FVector GetRandomPointInRadius(const FVector& Origin, float Radius);

    const char* ToString(EBotNavMode Mode);
}
