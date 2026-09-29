#pragma once
#include "../../../pch.h"
#include "../BotCompat.h"
#include "BotNavigation.h"

class FBotController;

enum class EBotMoveGoal : uint8
{
    None,
    Location, // MoveToLocation
    Actor,    // MoveToTarget: go to an actor and arrive
    Follow,   // FollowTarget: stay within FollowDistance of an actor indefinitely
};

enum class EBotMoveStatus : uint8
{
    Idle,
    Moving,
    Arrived, // Location/Actor goal reached. Follow goals report Moving or Arrived (in range) repeatedly.
    Failed,  // Goal gone, unreachable, or stuck too many times.
};

// Per-bot movement goals on top of IBotNavigation.
//
// Behaviors say *where* to go (MoveToLocation / MoveToActor / Follow); this class decides *how*:
// which navigation mode to use, when to re-path, when the goal counts as reached, what speed to use,
// and what to do when the pawn stops making progress (jump, then give up). Behaviors read Status to
// decide what to do next; they never talk to the navigation layer directly.
//
// Higher-level goals from the spec (MoveToLoot, MoveToSafePosition, MoveToCombatPosition, SearchArea)
// are behaviors choosing locations/actors for these primitives.
class FBotMovement
{
public:
    explicit FBotMovement(FBotController& InBot);

    void MoveToLocation(const FVector& Location, float AcceptanceRadius = -1.f);
    void MoveToActor(AActor* Target, float AcceptanceRadius = -1.f);
    void Follow(AActor* Target, float Distance = -1.f);
    void Stop();

    // Think-rate update (FBotSystemConfig::ThinkInterval).
    void Update(double Now);

    // Frame-rate update, only does work for navigation modes that need continuous input.
    void TickFrame();

    EBotMoveStatus GetStatus() const { return Status; }
    EBotMoveGoal GetGoalType() const { return GoalType; }
    EBotNavMode GetNavMode() const { return NavMode; }
    bool HasGoal() const { return GoalType != EBotMoveGoal::None; }

    // Current goal position (actor goals: last known actor location).
    const FVector& GetGoalLocation() const { return GoalLocation; }

    // Speed override. When not set, speed is chosen from distance (sprint when far).
    void SetSpeedOverride(bool bEnabled, EBotMoveSpeed Speed = EBotMoveSpeed::Run);

private:
    void SetGoal(EBotMoveGoal Type, AActor* Target, const FVector& Location, float Radius);
    bool RefreshGoalLocation();
    void IssueMove(double Now);
    void UpdateSpeed(double Distance);
    void CheckProgress(double Now, double Distance);

    FBotController& Bot;

    EBotMoveGoal GoalType = EBotMoveGoal::None;
    EBotMoveStatus Status = EBotMoveStatus::Idle;
    EBotNavMode NavMode = EBotNavMode::None;

    TWeakObjectPtr<AActor> GoalActor;
    FVector GoalLocation{};
    float GoalRadius = 0.f;

    bool bRequestActive = false;
    double LastIssueTime = 0.0;

    double NextProgressCheckTime = 0.0;
    FVector LastProgressLocation{};
    int StuckChecks = 0;
    bool bJumpPressed = false;

    bool bSpeedOverride = false;
    EBotMoveSpeed SpeedOverride = EBotMoveSpeed::Run;
    EBotMoveSpeed CurrentSpeed = EBotMoveSpeed::Run;
    bool bSpeedSupported = true;
};
