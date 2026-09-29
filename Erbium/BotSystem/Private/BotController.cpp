#include "pch.h"
#include "../Public/BotController.h"
#include "../Public/BotCompat.h"
#include "../Public/BotEngineTypes.h"
#include "../Public/BotLog.h"
#include "../../FortniteGame/Public/FortPlayerControllerAthena.h"
#include "../../FortniteGame/Public/FortGameMode.h"

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
    , Builder(*this)
    , Inventory(*this)
    , Combat(*this)
    , Brain(*this)
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
    Builder.Initialize();
    SetAIState(CanMove() ? Config.DefaultAIState : EBotAIState::Idle);
}

void FBotController::OnPawnDied(double Now)
{
    if (LifeState != EBotLifeState::Active)
        return;

    LifeState = EBotLifeState::Dead;
    DeathTime = Now;
    Combat.Reset();
    Movement.Stop();
    BotLootWorld::ReleaseClaims(Id);
    SetAIState(EBotAIState::Dead);

    if (ControllerKind == EBotControllerKind::AIController)
        HandleAIDeath();

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

    if (auto CurrentPawn = GetPawn())
        LastKnownLocation = CurrentPawn->K2_GetActorLocation();

    // Order matters: decisions use fresh perception, behaviors act on the decision, movement executes.
    Perception.Update(Now);
    TrackDamage(Now);
    Brain.Update(Now);
    EvaluateBuildDecision(Now);

    if (Behavior)
        Behavior->Tick(*this, DeltaSeconds);

    Movement.Update(Now);
}

void FBotController::TickFrame(double Now, float DeltaSeconds)
{
    Movement.TickFrame();

    if (GetAIState() == EBotAIState::Combat)
        Combat.UpdateAim(Now, DeltaSeconds);
}

void FBotController::IgnoreLoot(AActor* Target, double Seconds)
{
    if (Target)
        IgnoredLoot[Target->Index] = LastThinkTime + Seconds;
}

bool FBotController::IsLootIgnored(int32 ObjectIndex) const
{
    auto Found = IgnoredLoot.find(ObjectIndex);
    return Found != IgnoredLoot.end() && Found->second > LastThinkTime;
}

void FBotController::HandleAIDeath()
{
    // The pawn may already be flagged for destruction; the object itself is still valid this frame.
    auto DeadPawn = Pawn.Get();

    Inventory.DropAll(LastKnownLocation);

    auto World = UWorld::GetWorld();
    auto GameMode = World ? (AFortGameMode*)World->AuthorityGameMode : nullptr;
    if (!GameMode)
        return;

    // Placement first (counts this bot), then it stops counting, then check whether someone won.
    BotCompat::SetPlacement(GameMode, GetPlayerState());
    if (bRegisteredAlive && GetController())
    {
        BotCompat::UnregisterAlive(GameMode, GetController(), ControllerKind);
        bRegisteredAlive = false;
    }

    auto Killer = BotCompat::CreditBotKill(DeadPawn, GetPlayerState());
    BotCompat::CheckForWinner(GameMode, Killer);
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

void FBotController::TrackDamage(double Now)
{
    auto CurrentPawn = GetPawn();
    if (!CurrentPawn)
        return;

    auto HealthAndShield = CurrentPawn->GetHealth() + CurrentPawn->GetShield();
    if (LastHealthAndShield >= 0.f && HealthAndShield < LastHealthAndShield - 0.5f)
        LastDamageTime = Now;

    LastHealthAndShield = HealthAndShield;
}

static float RandomUnit()
{
    return (float)rand() / (float)RAND_MAX;
}

void FBotController::EvaluateBuildDecision(double Now)
{
    if (Now < NextBuildDecisionTime || !IsAlive() || !CanMove() || !Config.bCanBuild || !FBotSystemConfig::bAllowBuilding)
        return;

    NextBuildDecisionTime = Now + Config.BuildDecisionInterval;

    // Only interrupt "ordinary" states; never an ongoing build, heal or escape. Combat is included:
    // shoot -> build cover -> back to Combat (reposition, aim, shoot) is Phase 4 combat building.
    auto State = GetAIState();
    if (State != EBotAIState::Idle && State != EBotAIState::Searching && State != EBotAIState::Moving && State != EBotAIState::Following && State != EBotAIState::Looting &&
        State != EBotAIState::Combat)
        return;

    if (!Builder.IsReady(Now))
        return;

    auto CurrentPawn = GetPawn();
    if (!CurrentPawn)
        return;

    auto Location = CurrentPawn->K2_GetActorLocation();

    // Obstacle: movement gave up because it stopped making progress -> ramp towards where it was going.
    FVector StuckGoal{};
    if (Config.bBuildRampsWhenStuck && Movement.ConsumeStuckFailure(StuckGoal, Now, 2.0))
    {
        FVector Direction(StuckGoal.X - Location.X, StuckGoal.Y - Location.Y, 0.0);
        RequestBuild(EBotBuildPiece::Ramp, Direction, EBotBuildReason::Obstacle, &StuckGoal);
        return;
    }

    // Defensive: recently hit and an enemy is in sight -> wall between us and the nearest one.
    // Uses only what a player knows: own health dropping and enemies it can actually see.
    if (LastDamageTime > 0.0 && Now - LastDamageTime <= Config.DamageMemory)
    {
        auto Enemy = Perception.GetNearestVisibleEnemy();
        if (Enemy && RandomUnit() < Config.DefensiveBuildChance)
        {
            FVector Direction(Enemy->Location.X - Location.X, Enemy->Location.Y - Location.Y, 0.0);
            RequestBuild(EBotBuildPiece::Wall, Direction, EBotBuildReason::Defensive);
        }
    }
}

bool FBotController::RequestBuild(EBotBuildPiece Piece, const FVector& Direction, EBotBuildReason Reason, const FVector* ResumeGoal)
{
    if (!IsAlive() || !CanMove() || GetAIState() == EBotAIState::Building)
        return false;

    if (fabs(Direction.X) < 1.0 && fabs(Direction.Y) < 1.0)
        return false;

    Builder.Pending = FBotBuildRequest{};
    Builder.Pending.Piece = Piece;
    Builder.Pending.Direction = FVector(Direction);
    Builder.Pending.Reason = Reason;
    Builder.Pending.ReturnState = GetAIState() == EBotAIState::None ? EBotAIState::Searching : GetAIState();
    if (ResumeGoal)
    {
        Builder.Pending.bHasResumeGoal = true;
        Builder.Pending.ResumeGoal = FVector(*ResumeGoal);
    }
    Builder.bHasPending = true;

    SetAIState(EBotAIState::Building);
    return true;
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
