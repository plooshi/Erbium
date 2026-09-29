#include "pch.h"
#include "../../Public/Behaviors/BotBehaviors.h"
#include "../../Public/BotController.h"
#include "../../Public/BotLog.h"
#include "../../Public/Navigation/BotNavigation.h"
#include "../../../FortniteGame/Public/FortPlayerPawnAthena.h"

static float RandomRange(float Min, float Max)
{
    return Min + (Max - Min) * ((float)rand() / (float)RAND_MAX);
}

// --- Idle -----------------------------------------------------------------------------------------

void FBotIdleBehavior::OnEnter(FBotController& Bot)
{
    Bot.GetMovement().Stop();
}

// --- Searching ------------------------------------------------------------------------------------

void FBotSearchingBehavior::OnEnter(FBotController& Bot)
{
    // Small random delay so bots spawned together do not all start walking on the same frame.
    PauseRemaining = RandomRange(0.f, 1.f);
}

void FBotSearchingBehavior::Tick(FBotController& Bot, float DeltaSeconds)
{
    if (Bot.GetMovement().GetStatus() == EBotMoveStatus::Moving)
        return;

    // Arrived, failed or never started: look around for a moment, then pick the next spot.
    PauseRemaining -= DeltaSeconds;
    if (PauseRemaining > 0.f)
        return;

    PickNewDestination(Bot);
    PauseRemaining = RandomRange(1.f, 3.f);
}

void FBotSearchingBehavior::OnExit(FBotController& Bot)
{
    Bot.GetMovement().Stop();
}

void FBotSearchingBehavior::PickNewDestination(FBotController& Bot)
{
    auto Pawn = Bot.GetPawn();
    if (!Pawn)
        return;

    auto Destination = BotNavigation::GetRandomPointInRadius(Pawn->K2_GetActorLocation(), Bot.GetConfig().SearchRadius);
    Bot.GetMovement().MoveToLocation(Destination);
}

// --- Moving ---------------------------------------------------------------------------------------

void FBotMovingBehavior::OnEnter(FBotController& Bot)
{
    Bot.GetMovement().MoveToLocation(Bot.GetCommandLocation());
}

void FBotMovingBehavior::Tick(FBotController& Bot, float DeltaSeconds)
{
    auto Status = Bot.GetMovement().GetStatus();

    if (Status == EBotMoveStatus::Arrived)
        return Bot.SetAIState(EBotAIState::Idle);

    if (Status == EBotMoveStatus::Failed || Status == EBotMoveStatus::Idle)
    {
        BOT_VERBOSE(BOT_CATEGORY_AI, "Bot #%u could not reach its destination.", Bot.GetId());
        return Bot.SetAIState(EBotAIState::Idle);
    }
}

void FBotMovingBehavior::OnExit(FBotController& Bot)
{
    Bot.GetMovement().Stop();
}

// --- Following ------------------------------------------------------------------------------------

void FBotFollowingBehavior::OnEnter(FBotController& Bot)
{
    Bot.GetMovement().Follow(Bot.GetCommandTarget());
}

void FBotFollowingBehavior::Tick(FBotController& Bot, float DeltaSeconds)
{
    auto Target = Bot.GetCommandTarget();
    if (!Target)
        return Bot.SetAIState(EBotAIState::Searching);

    // Stuck or lost the path: try again rather than giving up on the target.
    if (!Bot.GetMovement().HasGoal())
        Bot.GetMovement().Follow(Target);
}

void FBotFollowingBehavior::OnExit(FBotController& Bot)
{
    Bot.GetMovement().Stop();
}

// --- Dead -----------------------------------------------------------------------------------------

void FBotDeadBehavior::OnEnter(FBotController& Bot)
{
    Bot.GetMovement().Stop();
}
