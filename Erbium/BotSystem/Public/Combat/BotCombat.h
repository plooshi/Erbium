#pragma once
#include "../../../pch.h"

class FBotController;
class AFortPlayerPawnAthena;
struct FBotPerceivedPawn;

// Phase 2C: targeting, aiming and weapon handling for one bot.
//
// Only uses what a player could know: targets come from FBotPerception (in range, seen with real line
// of sight, remembered for TargetMemory seconds after losing sight). No health/shield of enemies is
// read -- in Battle Royale a player cannot see those either.
//
// Aiming is deliberately imperfect and configurable (spec 2.8):
//  - AimReactionTime: after acquiring a target the bot does not aim at it at all for a moment;
//  - AimTurnSpeed:    the aim point moves towards the target at a limited speed (no snapping);
//  - AimAccuracy:     a random error around the target, re-rolled regularly, scaled with distance;
//  - MaxAimDistance:  targets further away are ignored.
// The aim point is fed to the AI controller's focal point, which drives the control rotation that
// the weapon fires along.
class FBotCombat
{
public:
    explicit FBotCombat(FBotController& InBot);

    // Think rate: choose target, weapon, fire / reload decisions.
    void Update(double Now, float DeltaSeconds);

    // Frame rate while in combat: smooth aim towards the target.
    void UpdateAim(double Now, float DeltaSeconds);

    // Stop firing and release the aim (leaving combat / dying).
    void Reset();

    AFortPlayerPawnAthena* GetTarget() const;
    bool HasTarget() const { return GetTarget() != nullptr; }
    float GetTargetDistance() const { return TargetDistance; }
    // Live position while visible, otherwise where it was last seen.
    const FVector& GetTargetLocation() const { return TargetLocation; }
    bool IsTargetVisible() const { return bTargetVisible; }
    bool IsReloading() const { return bReloading; }
    bool IsOutOfAmmo() const { return bOutOfAmmo; }
    const char* GetActivity() const { return Activity; }

    // Chooses/keeps a target from perception. Returns true if there is one.
    bool SelectTarget(double Now);

private:
    void ChooseWeapon(double Now);
    void UpdateFire(double Now);
    void StartFiring(double Now);
    void StopFiring();

    FBotController& Bot;

    TWeakObjectPtr<AFortPlayerPawnAthena> Target;
    double TargetAcquiredTime = 0.0;
    double LastTargetSwitchTime = -1000.0;
    float TargetDistance = 0.f;
    bool bTargetVisible = false;
    FVector TargetLocation{};

    FVector AimPoint{};
    bool bAimInitialized = false;
    FVector AimError{};
    double NextAimErrorTime = 0.0;
    double LastAimUpdateTime = 0.0;
    float AimOffTarget = 1000.f; // cm between aim point and target, for the fire decision

    bool bFiring = false;
    double FireStateEndTime = 0.0;
    bool bReloading = false;
    double ReloadEndTime = 0.0;
    bool bOutOfAmmo = false;
    double NextWeaponCheckTime = 0.0;

    const char* Activity = "Idle";
};
