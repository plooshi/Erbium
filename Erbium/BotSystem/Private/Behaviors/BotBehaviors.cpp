#include "pch.h"
#include "../../Public/Behaviors/BotBehaviors.h"
#include "../../Public/BotController.h"
#include "../../Public/BotLog.h"
#include "../../Public/Navigation/BotNavigation.h"
#include "../../Public/Combat/BotWeapons.h"
#include "../../Public/Loot/BotLoot.h"
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

// --- Looting --------------------------------------------------------------------------------------

void FBotLootingBehavior::OnEnter(FBotController& Bot)
{
    auto Target = Bot.GetBrain().GetLootTarget().Pickup.Get();
    TimeLeft = 15.f;

    if (!Target || Target->bPickedUp || !BotLootWorld::Claim(Target, Bot.GetId()))
    {
        // Taken or claimed by someone else: let the brain choose again.
        Pickup = TWeakObjectPtr<AFortPickupAthena>();
        return;
    }

    Pickup = TWeakObjectPtr<AFortPickupAthena>(Target);
    Bot.GetMovement().MoveToActor(Target, 100.f);
}

void FBotLootingBehavior::Tick(FBotController& Bot, float DeltaSeconds)
{
    auto Target = Pickup.Get();
    if (!Target || Target->bActorIsBeingDestroyed || Target->bPickedUp)
        return Bot.SetAIState(EBotAIState::Searching);

    if (BotLoot::TryPickup(Bot, Target))
    {
        BotLoot::EquipBestLoadout(Bot);
        return Bot.SetAIState(EBotAIState::Searching);
    }

    TimeLeft -= DeltaSeconds;
    auto Status = Bot.GetMovement().GetStatus();

    // Unreachable (stuck, no path, or arrived but still out of reach e.g. on a roof): skip it for a while.
    if (TimeLeft <= 0.f || Status == EBotMoveStatus::Failed || Status == EBotMoveStatus::Arrived || Status == EBotMoveStatus::Idle)
    {
        Bot.IgnoreLoot(Target, 30.0);
        return Bot.SetAIState(EBotAIState::Searching);
    }
}

void FBotLootingBehavior::OnExit(FBotController& Bot)
{
    BotLootWorld::ReleaseClaims(Bot.GetId());
    Bot.GetMovement().Stop();
}

// --- Combat ---------------------------------------------------------------------------------------

// Preferred engagement distances (cm) per weapon type.
static void GetPreferredRange(FBotController& Bot, float& OutMin, float& OutMax)
{
    auto WeaponData = BotWeapons::GetWeaponData(BotWeapons::GetCurrentWeapon(Bot.GetPawn()));
    auto Category = BotItems::Classify(WeaponData);

    if (Category == EBotItemCategory::Melee || !WeaponData)
    {
        OutMin = 0.f;
        OutMax = 150.f;
        return;
    }

    switch (BotItems::GetWeaponRange(WeaponData))
    {
    case EBotWeaponRange::Close:
        OutMin = 200.f;
        OutMax = 900.f;
        break;
    case EBotWeaponRange::Long:
        OutMin = 2500.f;
        OutMax = 7000.f;
        break;
    default:
        OutMin = 800.f;
        OutMax = 3500.f;
        break;
    }
}

void FBotCombatBehavior::OnEnter(FBotController& Bot)
{
    Bot.GetMovement().Stop();
    // Fortnite pawns cannot fire while sprinting; run instead.
    Bot.GetMovement().SetSpeedOverride(true, EBotMoveSpeed::Run);
    Bot.SetAimLock(true);
    NextRepositionTime = Bot.GetNow() + Bot.GetConfig().RepositionInterval;
    NextMoveDecisionTime = 0.0;
}

void FBotCombatBehavior::Tick(FBotController& Bot, float DeltaSeconds)
{
    auto Now = Bot.GetNow();
    auto& Combat = Bot.GetCombat();
    Combat.Update(Now, DeltaSeconds);

    auto Target = Combat.GetTarget();
    auto Pawn = Bot.GetPawn();
    if (!Target || !Pawn)
        return Bot.SetAIState(EBotAIState::Searching);

    if (Now < NextMoveDecisionTime)
        return;
    NextMoveDecisionTime = Now + 0.5;

    auto& Movement = Bot.GetMovement();
    auto Self = Pawn->K2_GetActorLocation();
    auto TargetLocation = Combat.GetTargetLocation();
    auto Distance = Combat.GetTargetDistance();

    // Out of sight: go to where it was last seen.
    if (!Combat.IsTargetVisible())
    {
        Movement.MoveToLocation(TargetLocation, 200.f);
        return;
    }

    float MinRange, MaxRange;
    GetPreferredRange(Bot, MinRange, MaxRange);

    double DX = Self.X - TargetLocation.X, DY = Self.Y - TargetLocation.Y;
    double Length = (std::max)(sqrt(DX * DX + DY * DY), 1.0);
    DX /= Length;
    DY /= Length;

    if (Distance > MaxRange)
    {
        Movement.MoveToActor(Target, MaxRange * 0.8f);
        return;
    }

    if (Distance < MinRange)
    {
        auto Back = MinRange - Distance + 200.f;
        Movement.MoveToLocation(FVector(Self.X + DX * Back, Self.Y + DY * Back, Self.Z));
        return;
    }

    // In range: strafe sideways now and then instead of standing still (spec 2.10).
    if (Now >= NextRepositionTime)
    {
        auto& Config = Bot.GetConfig();
        NextRepositionTime = Now + Config.RepositionInterval * RandomRange(0.7f, 1.3f);

        auto Side = RandomRange(400.f, 800.f) * (rand() % 2 ? 1.f : -1.f);
        Movement.MoveToLocation(FVector(Self.X - DY * Side, Self.Y + DX * Side, Self.Z));
    }
}

void FBotCombatBehavior::OnExit(FBotController& Bot)
{
    Bot.GetCombat().Reset();
    Bot.SetAimLock(false);
    Bot.GetMovement().SetSpeedOverride(false);
    Bot.GetMovement().Stop();
}

// --- Healing --------------------------------------------------------------------------------------

void FBotHealingBehavior::OnEnter(FBotController& Bot)
{
    auto Pawn = Bot.GetPawn();
    auto& Inventory = Bot.GetInventory();
    auto& Config = Bot.GetConfig();
    auto Items = Inventory.GetItems();

    Bot.GetMovement().Stop();
    bUsing = false;
    TimeLeft = 12.f;
    ItemDefinition = nullptr;

    if (!Pawn)
        return;

    // Shield when shields are low and health is fine, otherwise health.
    const FBotItem* Item = nullptr;
    if (Pawn->GetShield() < Config.HealShieldThreshold && Pawn->GetHealth() >= Config.HealHealthThreshold)
        Item = Inventory.GetBestOf(EBotItemCategory::Shield, Items);
    if (!Item && Pawn->GetHealth() < Config.HealHealthThreshold)
        Item = Inventory.GetBestOf(EBotItemCategory::Healing, Items);
    if (!Item)
        Item = Inventory.GetBestOf(EBotItemCategory::Shield, Items);

    if (!Item || !Inventory.Equip(*Item))
        return;

    ItemGuid = Item->Guid;
    ItemDefinition = Item->Definition;
    CountBefore = Inventory.CountOf(ItemDefinition);

    auto Effect = BotItems::GetConsumableEffect(ItemDefinition);
    bool bEmulate = !Inventory.GetNativeInventory() && FBotSystemConfig::bEmulateConsumablesForVirtualInventory;
    EmulatedUseTime = bEmulate ? (Effect ? Effect->UseTime : 3.f) : -1.f;
}

void FBotHealingBehavior::Tick(FBotController& Bot, float DeltaSeconds)
{
    auto Pawn = Bot.GetPawn();
    auto& Inventory = Bot.GetInventory();

    auto Finish = [&]()
    {
        BotLoot::EquipBestLoadout(Bot);
        Bot.SetAIState(EBotAIState::Searching);
    };

    if (!ItemDefinition || !Pawn)
        return Finish();

    // Interrupted by damage: let the brain decide (fight / flee).
    if (Bot.GetLastDamageTime() > 0.0 && Bot.GetNow() - Bot.GetLastDamageTime() < 0.2)
        return Finish();

    TimeLeft -= DeltaSeconds;

    // Short equip delay, then hold "fire" -- how consumables are used.
    if (!bUsing && TimeLeft < 11.6f)
        bUsing = BotWeapons::StartFire(Bot) || EmulatedUseTime >= 0.f;

    if (EmulatedUseTime >= 0.f)
    {
        EmulatedUseTime -= DeltaSeconds;
        if (EmulatedUseTime > 0.f)
            return;

        if (auto Effect = BotItems::GetConsumableEffect(ItemDefinition))
        {
            float Health = Pawn->GetHealth(), Shield = Pawn->GetShield();
            if (Effect->Health > 0.f && Health < Effect->HealthCap)
            {
                float NewHealth = (std::min)(Health + Effect->Health, Effect->HealthCap);
                Pawn->SetHealth(NewHealth);
            }
            if (Effect->Shield > 0.f && Shield < Effect->ShieldCap)
            {
                float NewShield = (std::min)(Shield + Effect->Shield, Effect->ShieldCap);
                Pawn->SetShield(NewShield);
            }
        }
        Inventory.Consume(ItemGuid, 1);
        return Finish();
    }

    // The game consumed the item (or gave up): done.
    if (Inventory.CountOf(ItemDefinition) < CountBefore || TimeLeft <= 0.f)
        return Finish();
}

void FBotHealingBehavior::OnExit(FBotController& Bot)
{
    if (bUsing)
        BotWeapons::StopFire(Bot);
}

// --- Escaping -------------------------------------------------------------------------------------

void FBotEscapingBehavior::OnEnter(FBotController& Bot)
{
    auto Pawn = Bot.GetPawn();
    TimeLeft = 0.f;
    if (!Pawn)
        return;

    auto Self = Pawn->K2_GetActorLocation();
    auto Enemy = Bot.GetPerception().GetNearestVisibleEnemy();
    FVector Threat = Enemy ? FVector(Enemy->Location) : FVector(Bot.GetCombat().GetTargetLocation());

    double DX = Self.X - Threat.X, DY = Self.Y - Threat.Y;
    double Length = sqrt(DX * DX + DY * DY);
    if (Length < 1.0)
    {
        DX = RandomRange(-1.f, 1.f);
        DY = RandomRange(-1.f, 1.f);
        Length = (std::max)(sqrt(DX * DX + DY * DY), 0.01);
    }

    auto Distance = Bot.GetConfig().EscapeDistance;
    FVector Away(Self.X + DX / Length * Distance, Self.Y + DY / Length * Distance, Self.Z);

    Bot.GetMovement().SetSpeedOverride(true, EBotMoveSpeed::Sprint);
    Bot.GetMovement().MoveToLocation(BotNavigation::GetRandomPointInRadius(Away, 500.f));
    TimeLeft = 6.f;
}

void FBotEscapingBehavior::Tick(FBotController& Bot, float DeltaSeconds)
{
    TimeLeft -= DeltaSeconds;
    if (TimeLeft <= 0.f || Bot.GetMovement().GetStatus() != EBotMoveStatus::Moving)
        Bot.SetAIState(EBotAIState::Searching);
}

void FBotEscapingBehavior::OnExit(FBotController& Bot)
{
    Bot.GetMovement().SetSpeedOverride(false);
    Bot.GetMovement().Stop();
}

// --- Dead -----------------------------------------------------------------------------------------

void FBotDeadBehavior::OnEnter(FBotController& Bot)
{
    Bot.GetMovement().Stop();
}
