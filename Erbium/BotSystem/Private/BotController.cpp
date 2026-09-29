#include "pch.h"
#include "../Public/BotController.h"
#include "../Public/BotCompat.h"
#include "../Public/BotEngineTypes.h"
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
    , Movement(*this)
    , Perception(*this)
{
}

FBotController::~FBotController()
{
    // Behaviors may hold per-bot state; give them a chance to release it.
    if (Behavior)
        Behavior->OnExit(*this);
}

AController* FBotController::GetController() const
{
    return GetLiveActor(Controller);
}

AAIController* FBotController::GetAIController() const
{
    return ControllerKind == EBotControllerKind::AIController ? (AAIController*)GetController() : nullptr;
}

AFortPlayerControllerAthena* FBotController::GetPlayerController() const
{
    return ControllerKind == EBotControllerKind::PlayerController ? (AFortPlayerControllerAthena*)GetController() : nullptr;
}

AFortPlayerPawnAthena* FBotController::GetPawn() const
{
    return GetLiveActor(Pawn);
}

AFortPlayerStateAthena* FBotController::GetPlayerState() const
{
    return GetLiveActor(PlayerState);
}

AActor* FBotController::GetCommandTarget() const
{
    return GetLiveActor(CommandTarget);
}

void FBotController::BindController(AController* InController, EBotControllerKind InKind, AFortPlayerStateAthena* InPlayerState)
{
    Controller = TWeakObjectPtr<AController>(InController);
    ControllerKind = InKind;
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
    SetAIState(CanMove() ? Config.DefaultAIState : EBotAIState::Idle);
}

void FBotController::OnPawnDied(double Now)
{
    if (LifeState != EBotLifeState::Active)
        return;

    LifeState = EBotLifeState::Dead;
    DeathTime = Now;
    Movement.Stop();
    SetAIState(EBotAIState::Dead);

    BOT_LOG(BOT_CATEGORY_SYSTEM, "Bot #%u died.", Id);
}

bool FBotController::Validate(double Now)
{
    if (LifeState == EBotLifeState::PendingRemoval)
        return false;

    // Controller destroyed by something other than us (map travel, another system): nothing left to drive.
    auto CurrentController = GetController();
    if (!CurrentController)
    {
        BOT_LOG(BOT_CATEGORY_SYSTEM, "Bot #%u lost its controller.", Id);
        return false;
    }

    if (LifeState == EBotLifeState::Active)
    {
        auto CurrentPawn = GetPawn();

        // The game may swap pawns (e.g. aircraft -> skydiving). Track whatever the controller possesses now.
        if (auto Possessed = CurrentController->Pawn; Possessed && Possessed != CurrentPawn && !Possessed->bActorIsBeingDestroyed)
        {
            Pawn = TWeakObjectPtr<AFortPlayerPawnAthena>(Possessed);
            CurrentPawn = Possessed;
        }

        if (BotCompat::IsEliminated(CurrentController, ControllerKind, CurrentPawn))
            OnPawnDied(Now);
    }

    return true;
}

void FBotController::Think(double Now)
{
    auto DeltaSeconds = (float)(Now - LastThinkTime);
    LastThinkTime = Now;

    // Order matters: behaviors decide on fresh perception, movement executes what they decided.
    Perception.Update(Now);

    if (Behavior)
        Behavior->Tick(*this, DeltaSeconds);

    Movement.Update(Now);
}

void FBotController::TickFrame()
{
    Movement.TickFrame();
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

void FBotController::CommandMoveTo(const FVector& Location)
{
    if (!IsAlive())
        return;

    CommandLocation = FVector(Location);

    // Re-enter even if already Moving so the new destination is picked up.
    SetAIState(EBotAIState::Idle);
    SetAIState(EBotAIState::Moving);
}

void FBotController::CommandFollow(AActor* Target)
{
    if (!IsAlive() || !Target)
        return;

    CommandTarget = TWeakObjectPtr<AActor>(Target);
    SetAIState(EBotAIState::Idle);
    SetAIState(EBotAIState::Following);
}
