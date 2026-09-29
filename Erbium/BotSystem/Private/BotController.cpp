#include "pch.h"
#include "../Public/BotController.h"
#include "../Public/BotCompat.h"
#include "../Public/BotLog.h"
#include "../../FortniteGame/Public/FortPlayerControllerAthena.h"

template <typename T>
static T* GetLiveActor(const TWeakObjectPtr<T>& Weak)
{
    auto Actor = Weak.Get();
    return Actor && !Actor->bActorIsBeingDestroyed ? Actor : nullptr;
}

FBotController::FBotController(uint32 InId, const FBotConfig& InConfig)
    : Id(InId)
    , Config(InConfig)
{
}

FBotController::~FBotController()
{
    // Behaviors may hold per-bot state; give them a chance to release it.
    if (Behavior)
        Behavior->OnExit(*this);
}

AFortPlayerControllerAthena* FBotController::GetController() const
{
    return GetLiveActor(Controller);
}

AFortPlayerPawnAthena* FBotController::GetPawn() const
{
    return GetLiveActor(Pawn);
}

AFortPlayerStateAthena* FBotController::GetPlayerState() const
{
    return GetLiveActor(PlayerState);
}

void FBotController::BindController(AFortPlayerControllerAthena* InController, AFortPlayerStateAthena* InPlayerState)
{
    Controller = TWeakObjectPtr<AFortPlayerControllerAthena>(InController);
    PlayerState = TWeakObjectPtr<AFortPlayerStateAthena>(InPlayerState);
}

bool FBotController::OnPossessed(AFortPlayerPawnAthena* InPawn)
{
    auto PC = GetController();
    if (!PC || !InPawn || PC->Pawn != InPawn)
        return false;

    Pawn = TWeakObjectPtr<AFortPlayerPawnAthena>(InPawn);
    return true;
}

void FBotController::Activate(double Now)
{
    LifeState = EBotLifeState::Active;
    LastThinkTime = Now;
    SetAIState(EBotAIState::Idle);
}

void FBotController::OnPawnDied(double Now)
{
    if (LifeState != EBotLifeState::Active)
        return;

    LifeState = EBotLifeState::Dead;
    DeathTime = Now;
    SetAIState(EBotAIState::Dead);

    BOT_LOG(BOT_CATEGORY_SYSTEM, "Bot #%u died.", Id);
}

bool FBotController::Validate(double Now)
{
    if (LifeState == EBotLifeState::PendingRemoval)
        return false;

    // Controller destroyed by something other than us (map travel, another system): nothing left to drive.
    if (!GetController())
    {
        BOT_LOG(BOT_CATEGORY_SYSTEM, "Bot #%u lost its controller.", Id);
        return false;
    }

    if (LifeState == EBotLifeState::Active)
    {
        auto CurrentPawn = GetPawn();

        // The game may swap pawns (e.g. aircraft -> skydiving). Track whatever the controller possesses now.
        if (auto Possessed = GetController()->Pawn; Possessed && Possessed != CurrentPawn && !Possessed->bActorIsBeingDestroyed)
        {
            Pawn = TWeakObjectPtr<AFortPlayerPawnAthena>(Possessed);
            CurrentPawn = Possessed;
        }

        if (BotCompat::IsEliminated(GetController(), CurrentPawn))
            OnPawnDied(Now);
    }

    return true;
}

void FBotController::Think(double Now)
{
    auto DeltaSeconds = (float)(Now - LastThinkTime);
    LastThinkTime = Now;

    if (Behavior)
        Behavior->Tick(*this, DeltaSeconds);
}

void FBotController::SetAIState(EBotAIState NewState)
{
    auto OldState = GetAIState();
    if (OldState == NewState && Behavior)
        return;

    if (Behavior)
        Behavior->OnExit(*this);

    Behavior = CreateBotBehavior(NewState);

    if (Behavior)
        Behavior->OnEnter(*this);

    BOT_VERBOSE(BOT_CATEGORY_AI, "Bot #%u: %s -> %s", Id, ToString(OldState), ToString(NewState));
}
