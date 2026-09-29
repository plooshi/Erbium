#include "pch.h"
#include "../../Public/Navigation/BotMovement.h"
#include "../../Public/BotController.h"
#include "../../Public/BotEngineTypes.h"
#include "../../Public/BotLog.h"
#include "../../../FortniteGame/Public/FortPlayerPawnAthena.h"

// Vertical tolerance for "arrived": goals on a slope or a floor above/below still count within this.
static constexpr double ArrivalHeightTolerance = 300.0;

// Minimum time between re-issuing a move that the navigation layer reports as finished while the
// bot is not at its goal yet (prevents hammering a failing request every think).
static constexpr double MinReissueInterval = 0.5;

static double Distance2D(const FVector& A, const FVector& B)
{
    auto DX = A.X - B.X;
    auto DY = A.Y - B.Y;
    return sqrt(DX * DX + DY * DY);
}

FBotMovement::FBotMovement(FBotController& InBot)
    : Bot(InBot)
{
}

void FBotMovement::SetGoal(EBotMoveGoal Type, AActor* Target, const FVector& Location, float Radius)
{
    // Behaviors re-send their destination every decision (combat: every 0.5 s). The same goal again
    // must not restart the move: that would re-issue the navigation request and reset the stuck
    // detection each time, so a bot that cannot move would never be detected (or fall back to steering).
    if (Status == EBotMoveStatus::Moving && GoalType == Type && fabs(GoalRadius - Radius) < 1.f)
    {
        bool bSameTarget = Type == EBotMoveGoal::Location ? Distance2D(GoalLocation, Location) < 100.0 && fabs(GoalLocation.Z - Location.Z) < 200.0
                                                           : GoalActor.Get() == Target;
        if (bSameTarget)
        {
            if (Type == EBotMoveGoal::Location)
                GoalLocation = FVector(Location);
            return;
        }
    }

    GoalType = Type;
    GoalActor = TWeakObjectPtr<AActor>(Target);
    GoalLocation = FVector(Location);
    GoalRadius = Radius;
    Status = EBotMoveStatus::Moving;

    // Issue on the next Update (same think, the behavior tick runs first).
    bRequestActive = false;
    LastIssueTime = -1000.0;

    StuckChecks = 0;
    NextProgressCheckTime = 0.0;
    if (auto Pawn = Bot.GetPawn())
        LastProgressLocation = Pawn->K2_GetActorLocation();
}

void FBotMovement::MoveToLocation(const FVector& Location, float AcceptanceRadius)
{
    SetGoal(EBotMoveGoal::Location, nullptr, Location, AcceptanceRadius > 0.f ? AcceptanceRadius : Bot.GetConfig().AcceptanceRadius);
}

void FBotMovement::MoveToActor(AActor* Target, float AcceptanceRadius)
{
    if (!Target)
        return;

    SetGoal(EBotMoveGoal::Actor, Target, Target->K2_GetActorLocation(), AcceptanceRadius > 0.f ? AcceptanceRadius : Bot.GetConfig().AcceptanceRadius);
}

void FBotMovement::Follow(AActor* Target, float Distance)
{
    if (!Target)
        return;

    SetGoal(EBotMoveGoal::Follow, Target, Target->K2_GetActorLocation(), Distance > 0.f ? Distance : Bot.GetConfig().FollowDistance);
}

void FBotMovement::Stop()
{
    if (bRequestActive)
        BotNavigation::Get(NavMode)->Stop(Bot);

    GoalType = EBotMoveGoal::None;
    GoalActor = TWeakObjectPtr<AActor>();
    Status = EBotMoveStatus::Idle;
    bRequestActive = false;
}

bool FBotMovement::ConsumeStuckFailure(FVector& OutGoal, double Now, double MaxAge)
{
    if (!bStuckFailurePending)
        return false;

    bStuckFailurePending = false;
    if (Now - StuckFailureTime > MaxAge)
        return false;

    OutGoal = FVector(StuckFailureGoal);
    return true;
}

void FBotMovement::SetSpeedOverride(bool bEnabled, EBotMoveSpeed Speed)
{
    bSpeedOverride = bEnabled;
    SpeedOverride = Speed;
}

bool FBotMovement::RefreshGoalLocation()
{
    if (GoalType == EBotMoveGoal::Location)
        return true;

    auto Actor = GoalActor.Get();
    if (!Actor || Actor->bActorIsBeingDestroyed)
        return false;

    GoalLocation = Actor->K2_GetActorLocation();
    return true;
}

void FBotMovement::IssueMove(double Now)
{
    auto Mode = !Bot.CanMove() ? EBotNavMode::None : (ForcedNavMode != EBotNavMode::None ? ForcedNavMode : BotNavigation::GetPreferredMode());
    auto Actor = GoalType != EBotMoveGoal::Location ? GoalActor.Get() : nullptr;

    // Follow: aim a bit inside the follow distance so the bot ends up within it.
    auto Radius = GoalType == EBotMoveGoal::Follow ? GoalRadius * 0.8f : GoalRadius;

    while (Mode != EBotNavMode::None)
    {
        auto Navigation = BotNavigation::Get(Mode);
        auto Result = Actor ? Navigation->MoveToActor(Bot, Actor, Radius) : Navigation->MoveToLocation(Bot, GoalLocation, Radius);

        BotNavigation::ReportResult(Mode, Result != EBotNavRequestResult::Failed);

        if (Result != EBotNavRequestResult::Failed)
            break;

        Mode = ForcedNavMode != EBotNavMode::None ? EBotNavMode::None : BotNavigation::GetFallbackMode(Mode);
    }

    if (Mode != NavMode)
    {
        BOT_VERBOSE(BOT_CATEGORY_NAV, "Bot #%u navigation: %s -> %s", Bot.GetId(), BotNavigation::ToString(NavMode), BotNavigation::ToString(Mode));
        NavMode = Mode;
    }

    LastIssueTime = Now;
    bRequestActive = Mode != EBotNavMode::None;

    if (!bRequestActive)
    {
        Status = EBotMoveStatus::Failed;
        GoalType = EBotMoveGoal::None;
    }
}

void FBotMovement::UpdateSpeed(double Distance)
{
    if (!bSpeedSupported)
        return;

    auto Speed = bSpeedOverride ? SpeedOverride : (Bot.GetConfig().bCanSprint && Distance > Bot.GetConfig().SprintDistance ? EBotMoveSpeed::Sprint : EBotMoveSpeed::Run);

    if (Speed == CurrentSpeed)
        return;

    if (auto Pawn = Bot.GetPawn())
    {
        bSpeedSupported = BotCompat::SetMovementSpeed(Pawn, Speed);
        CurrentSpeed = Speed;
    }
}

void FBotMovement::CheckProgress(double Now, double Distance)
{
    auto Pawn = Bot.GetPawn();
    if (!Pawn || Now < NextProgressCheckTime)
        return;

    auto& Config = Bot.GetConfig();
    auto Location = Pawn->K2_GetActorLocation();

    // First check after a new goal only records the starting point.
    if (NextProgressCheckTime != 0.0)
    {
        if (Distance2D(Location, LastProgressLocation) < Config.StuckDistance)
        {
            StuckChecks++;
            BOT_VERBOSE(BOT_CATEGORY_NAV, "Bot #%u made no progress (%d/%d).", Bot.GetId(), StuckChecks, Config.MaxStuckChecks);

            if (StuckChecks >= Config.MaxStuckChecks)
            {
                BotNavigation::ReportProgress(NavMode, false);
                bStuckFailurePending = true;
                StuckFailureTime = Now;
                StuckFailureGoal = FVector(GoalLocation);

                BotNavigation::Get(NavMode)->Stop(Bot);
                Status = EBotMoveStatus::Failed;
                GoalType = EBotMoveGoal::None;
                bRequestActive = false;
                return;
            }

            // Hop over whatever is in the way and ask for a fresh path.
            if (Config.bCanJump)
            {
                ((ACharacter*)Pawn)->Jump();
                bJumpPressed = true;
            }
            bRequestActive = false;
        }
        else
        {
            StuckChecks = 0;
            BotNavigation::ReportProgress(NavMode, true);
        }
    }

    LastProgressLocation = Location;
    NextProgressCheckTime = Now + Config.StuckCheckInterval;
}

void FBotMovement::Update(double Now)
{
    auto Pawn = Bot.GetPawn();

    if (bJumpPressed && Pawn)
    {
        ((ACharacter*)Pawn)->StopJumping();
        bJumpPressed = false;
    }

    if (GoalType == EBotMoveGoal::None)
        return;

    if (!Pawn || !RefreshGoalLocation())
    {
        Stop();
        Status = EBotMoveStatus::Failed;
        return;
    }

    auto Location = Pawn->K2_GetActorLocation();
    auto Distance = Distance2D(Location, GoalLocation);
    bool bInRange = Distance <= GoalRadius && fabs(Location.Z - GoalLocation.Z) <= ArrivalHeightTolerance;

    if (bInRange)
    {
        if (bRequestActive)
            BotNavigation::Get(NavMode)->Stop(Bot);
        bRequestActive = false;

        if (GoalType == EBotMoveGoal::Follow)
        {
            // Keep the goal: start moving again as soon as the target walks away.
            Status = EBotMoveStatus::Arrived;
            StuckChecks = 0;
            NextProgressCheckTime = 0.0;
            return;
        }

        Status = EBotMoveStatus::Arrived;
        GoalType = EBotMoveGoal::None;
        return;
    }

    Status = EBotMoveStatus::Moving;

    auto& Config = Bot.GetConfig();
    bool bMovingGoal = GoalType != EBotMoveGoal::Location;
    // The mode in use was dropped (see BotNavigation::ReportProgress): re-issue with the new one.
    bool bModeDropped = bRequestActive && BotNavigation::IsDisabled(NavMode);
    if (bModeDropped)
        BotNavigation::Get(NavMode)->Stop(Bot);
    bool bNeedsIssue = bModeDropped || !bRequestActive || (bMovingGoal && Now - LastIssueTime >= Config.RepathInterval) ||
                       (Now - LastIssueTime >= MinReissueInterval && !BotNavigation::Get(NavMode)->IsMoving(Bot));

    if (bNeedsIssue)
    {
        IssueMove(Now);
        if (Status == EBotMoveStatus::Failed)
            return;
    }

    UpdateSpeed(Distance);
    CheckProgress(Now, Distance);
}

void FBotMovement::TickFrame()
{
    if (Status != EBotMoveStatus::Moving || !bRequestActive)
        return;

    auto Navigation = BotNavigation::Get(NavMode);
    if (!Navigation->NeedsFrameTick())
        return;

    // Actor goals: GoalLocation is refreshed at think rate, which is plenty for steering.
    Navigation->TickFrame(Bot, GoalLocation, GoalType == EBotMoveGoal::Follow ? GoalRadius * 0.8f : GoalRadius);
}
