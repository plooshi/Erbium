#include "pch.h"
#include "../../Public/Combat/BotCombat.h"
#include "../../Public/BotController.h"
#include "../../Public/BotEngineTypes.h"
#include "../../Public/BotLog.h"
#include "../../Public/Combat/BotWeapons.h"
#include "../../../FortniteGame/Public/FortWeapon.h"

static constexpr double DegToRad = 3.14159265358979323846 / 180.0;

static float RandomRange(float Min, float Max)
{
    return Min + (Max - Min) * ((float)rand() / (float)RAND_MAX);
}

static double Length(double X, double Y, double Z)
{
    return sqrt(X * X + Y * Y + Z * Z);
}

FBotCombat::FBotCombat(FBotController& InBot)
    : Bot(InBot)
{
}

AFortPlayerPawnAthena* FBotCombat::GetTarget() const
{
    auto Pawn = Target.Get();
    return Pawn && !Pawn->bActorIsBeingDestroyed ? Pawn : nullptr;
}

bool FBotCombat::SelectTarget(double Now)
{
    auto& Config = Bot.GetConfig();
    auto Current = GetTarget();

    const FBotPerceivedPawn* Best = nullptr;
    const FBotPerceivedPawn* CurrentEntry = nullptr;
    float BestScore = 0.f;

    for (auto& Perceived : Bot.GetPerception().GetPawns())
    {
        if (!Perceived.bIsEnemy || Perceived.Distance > Config.MaxAimDistance)
            continue;

        bool bRecentlySeen = Perceived.bVisible || (Perceived.LastSeenTime > 0.0 && Now - Perceived.LastSeenTime <= Config.TargetMemory);
        if (!bRecentlySeen)
            continue;

        auto Pawn = Perceived.Pawn.Get();
        if (!Pawn)
            continue;

        // Closer and visible first; a small bonus keeps the current target (no flip-flopping).
        float Score = (Perceived.bVisible ? 1.f : 0.5f) * (1.f - Perceived.Distance / Config.MaxAimDistance) + 0.05f;
        if (Pawn == Current)
        {
            Score += 0.3f;
            CurrentEntry = &Perceived;
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            Best = &Perceived;
        }
    }

    // Keep the current target during the switch delay if it is still known.
    if (Best && Current && Best->Pawn.Get() != Current && CurrentEntry && Now - LastTargetSwitchTime < Config.TargetSwitchDelay)
        Best = CurrentEntry;

    if (!Best)
    {
        if (Current)
            BOT_VERBOSE(BOT_CATEGORY_COMBAT, "Bot #%u lost its target.", Bot.GetId());
        Target = TWeakObjectPtr<AFortPlayerPawnAthena>();
        bTargetVisible = false;
        return false;
    }

    auto BestPawn = Best->Pawn.Get();
    if (BestPawn != Current)
    {
        Target = Best->Pawn;
        TargetAcquiredTime = Now;
        LastTargetSwitchTime = Now;
        bAimInitialized = false;
        BOT_VERBOSE(BOT_CATEGORY_COMBAT, "Bot #%u targets a pawn %.0f m away.", Bot.GetId(), Best->Distance / 100.f);
    }

    TargetDistance = Best->Distance;
    bTargetVisible = Best->bVisible;

    // Only follow the target's real position while it is visible; otherwise aim at where it was seen.
    if (bTargetVisible)
        TargetLocation = BestPawn->K2_GetActorLocation();
    else if (BestPawn != Current)
        TargetLocation = FVector(Best->Location);

    return true;
}

void FBotCombat::ChooseWeapon(double Now)
{
    if (Now < NextWeaponCheckTime || bFiring || bReloading)
        return;

    NextWeaponCheckTime = Now + 1.0;

    auto& Inventory = Bot.GetInventory();
    auto Items = Inventory.GetItems();
    auto Best = Inventory.GetBestWeaponFor(TargetDistance, Items);

    bOutOfAmmo = Best == nullptr;
    if (!Best)
        Best = Inventory.GetBestOf(EBotItemCategory::Melee, Items);

    if (Best && !SameGuid(Best->Guid, Inventory.GetEquippedGuid()) && Inventory.Equip(*Best))
    {
        // Equip time: do not fire for a moment.
        FireStateEndTime = Now + 0.4;
        BOT_VERBOSE(BOT_CATEGORY_COMBAT, "Bot #%u switches to %s", Bot.GetId(), BotItems::GetName(Best->Definition).c_str());
    }
}

void FBotCombat::StartFiring(double Now)
{
    if (BotWeapons::StartFire(Bot))
    {
        bFiring = true;
        FireStateEndTime = Now + RandomRange(0.25f, 0.9f);
        Activity = "Firing";
    }
}

void FBotCombat::StopFiring()
{
    if (bFiring)
        BotWeapons::StopFire(Bot);
    bFiring = false;
}

void FBotCombat::UpdateFire(double Now)
{
    auto& Config = Bot.GetConfig();
    auto Weapon = BotWeapons::GetCurrentWeapon(Bot.GetPawn());
    auto WeaponData = BotWeapons::GetWeaponData(Weapon);
    if (!Weapon || !WeaponData)
    {
        StopFiring();
        Activity = "NoWeapon";
        return;
    }

    auto& Inventory = Bot.GetInventory();
    auto Category = BotItems::Classify(WeaponData);
    auto Loaded = BotWeapons::GetLoadedAmmo(Weapon);

    // Reloading.
    if (bReloading)
    {
        Activity = "Reloading";
        if (Now < ReloadEndTime)
            return;

        bReloading = false;
        for (auto& Item : Inventory.GetItems())
            if (SameGuid(Item.Guid, Weapon->ItemEntryGuid))
                Inventory.ReloadVirtual(Item); // no-op (-1) for native inventories: the game reloaded
        return;
    }

    if (Category == EBotItemCategory::Weapon && Loaded == 0)
    {
        StopFiring();

        if (Inventory.GetReserveAmmo(WeaponData) > 0)
        {
            BotWeapons::StartReload(Bot);
            bReloading = true;
            ReloadEndTime = Now + BotWeapons::GetReloadTime(WeaponData);
            Activity = "Reloading";
        }
        else
        {
            // Empty and nothing to reload with: pick another weapon right away.
            bOutOfAmmo = true;
            NextWeaponCheckTime = 0.0;
        }
        return;
    }

    if (Category == EBotItemCategory::Weapon && Loaded > 0)
        Inventory.SyncLoadedAmmo(Weapon->ItemEntryGuid, Loaded);

    // Fire decision: target visible, reaction time passed, aim close enough, in range.
    auto MaxRange = Category == EBotItemCategory::Melee ? 250.f : Config.MaxAimDistance;
    auto Tolerance = (std::max)(60.0, TargetDistance * tan(Config.FireToleranceDegrees * DegToRad));
    bool bCanShoot = bTargetVisible && Now - TargetAcquiredTime >= Config.AimReactionTime && AimOffTarget <= Tolerance && TargetDistance <= MaxRange && BotWeapons::IsFiringSupported();

    // Bursts with short pauses instead of holding the trigger forever.
    if (bFiring)
    {
        if (!bCanShoot || Now >= FireStateEndTime)
        {
            StopFiring();
            FireStateEndTime = Now + RandomRange(0.15f, 0.45f);
            Activity = "Aiming";
        }
    }
    else if (bCanShoot && Now >= FireStateEndTime)
        StartFiring(Now);
    else
        Activity = bTargetVisible ? "Aiming" : "Searching";
}

void FBotCombat::Update(double Now, float DeltaSeconds)
{
    if (!SelectTarget(Now))
    {
        StopFiring();
        Activity = "NoTarget";
        return;
    }

    ChooseWeapon(Now);
    UpdateFire(Now);
}

void FBotCombat::UpdateAim(double Now, float DeltaSeconds)
{
    auto Pawn = Bot.GetPawn();
    auto AI = Bot.GetAIController();
    auto TargetPawn = GetTarget();
    auto& Config = Bot.GetConfig();

    if (!Pawn || !AI || !TargetPawn)
        return;

    // Reaction time: do not track a freshly acquired target yet.
    if (Now - TargetAcquiredTime < Config.AimReactionTime)
        return;

    auto Self = Pawn->K2_GetActorLocation();
    if (bTargetVisible)
        TargetLocation = TargetPawn->K2_GetActorLocation();

    auto Distance = Length(TargetLocation.X - Self.X, TargetLocation.Y - Self.Y, TargetLocation.Z - Self.Z);

    // Error around the target, re-rolled regularly, bigger for less accurate bots and far targets.
    if (Now >= NextAimErrorTime)
    {
        NextAimErrorTime = Now + RandomRange(0.35f, 0.6f);
        auto Magnitude = (1.f - Config.AimAccuracy) * ((float)Distance * 0.06f + 30.f);
        AimError = FVector(RandomRange(-1.f, 1.f) * Magnitude, RandomRange(-1.f, 1.f) * Magnitude, RandomRange(-0.6f, 0.6f) * Magnitude);
    }

    FVector Desired(TargetLocation.X + AimError.X, TargetLocation.Y + AimError.Y, TargetLocation.Z + Config.AimHeightOffset + AimError.Z);

    if (!bAimInitialized)
    {
        // Start off to the side, as if turning towards the target from elsewhere.
        auto Side = Distance * 0.25;
        AimPoint = FVector(Desired.X + RandomRange(-1.f, 1.f) * Side, Desired.Y + RandomRange(-1.f, 1.f) * Side, Desired.Z);
        bAimInitialized = true;
    }

    // Limited tracking speed: the aim point moves at most AimTurnSpeed (degrees/s) at this distance.
    auto Step = Distance * Config.AimTurnSpeed * DegToRad * (std::min)(DeltaSeconds, 0.1f);
    auto DX = Desired.X - AimPoint.X, DY = Desired.Y - AimPoint.Y, DZ = Desired.Z - AimPoint.Z;
    auto Remaining = Length(DX, DY, DZ);

    if (Remaining <= Step || Remaining < 1.0)
        AimPoint = FVector(Desired);
    else
        AimPoint = FVector(AimPoint.X + DX / Remaining * Step, AimPoint.Y + DY / Remaining * Step, AimPoint.Z + DZ / Remaining * Step);

    AimOffTarget = (float)Length(AimPoint.X - TargetLocation.X, AimPoint.Y - TargetLocation.Y, AimPoint.Z - (TargetLocation.Z + Config.AimHeightOffset));
    TargetDistance = (float)Distance;

    FVector Focal = AimPoint;
    AI->K2_SetFocalPoint(Focal);
}

void FBotCombat::Reset()
{
    StopFiring();
    bReloading = false;
    bAimInitialized = false;
    Target = TWeakObjectPtr<AFortPlayerPawnAthena>();
    bTargetVisible = false;
    Activity = "Idle";

    if (auto AI = Bot.GetAIController())
        AI->K2_ClearFocus();
}
