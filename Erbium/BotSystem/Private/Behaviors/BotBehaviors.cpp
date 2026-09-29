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

// --- Building -------------------------------------------------------------------------------------

// How long a bot may spend walking over a ramp before giving up and carrying on.
static constexpr float MaxTraverseTime = 6.f;

void FBotBuildingBehavior::OnEnter(FBotController& Bot)
{
    Bot.GetMovement().Stop();
    Phase = EPhase::Reacting;
    Timer = Bot.GetConfig().BuildReactionTime * RandomRange(0.5f, 1.5f);
}

void FBotBuildingBehavior::Tick(FBotController& Bot, float DeltaSeconds)
{
    auto& Builder = Bot.GetBuilder();
    Timer -= DeltaSeconds;

    switch (Phase)
    {
    case EPhase::Reacting:
    {
        if (Timer > 0.f)
            return;

        if (!Builder.bHasPending)
            return Finish(Bot);

        auto Request = Builder.Pending;
        auto Result = Builder.Build(Request, Bot.GetNow());
        BOT_VERBOSE(BOT_CATEGORY_BUILDING, "Bot #%u build %s: %s", Bot.GetId(), BotBuildCatalog::ToString(Request.Piece), FBotBuilder::ToString(Result));

        if (Result != EBotBuildResult::Placed)
            return Finish(Bot);

        if (Request.Reason == EBotBuildReason::Obstacle && Request.bHasResumeGoal)
        {
            // Walk over the ramp towards the goal that was blocked.
            Phase = EPhase::Traversing;
            Timer = MaxTraverseTime;
            Bot.GetMovement().MoveToLocation(Request.ResumeGoal);
            return;
        }

        if (Request.Reason == EBotBuildReason::Defensive)
        {
            Phase = EPhase::Holding;
            Timer = Bot.GetConfig().DefensiveHoldTime;
            return;
        }

        return Finish(Bot);
    }
    case EPhase::Holding:
        if (Timer <= 0.f)
            Finish(Bot);
        return;
    case EPhase::Traversing:
        if (Timer <= 0.f || Bot.GetMovement().GetStatus() != EBotMoveStatus::Moving)
            Finish(Bot);
        return;
    }
}

void FBotBuildingBehavior::OnExit(FBotController& Bot)
{
    Bot.GetBuilder().bHasPending = false;
    Bot.GetMovement().Stop();
}

void FBotBuildingBehavior::Finish(FBotController& Bot)
{
    auto ReturnState = Bot.GetBuilder().Pending.ReturnState;
    if (ReturnState == EBotAIState::Building || ReturnState == EBotAIState::Dead || ReturnState == EBotAIState::None)
        ReturnState = EBotAIState::Searching;

    Bot.SetAIState(ReturnState);
}

// --- Dead -----------------------------------------------------------------------------------------

void FBotDeadBehavior::OnEnter(FBotController& Bot)
{
    Bot.GetMovement().Stop();
}
