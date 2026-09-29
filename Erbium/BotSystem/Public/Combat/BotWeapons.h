#pragma once
#include "../../../pch.h"

class FBotController;
class AFortWeapon;
class AFortPlayerPawnAthena;
class UFortItemDefinition;

// Weapon-level compatibility layer (Phase 2C).
//
// Bots use weapons through the same gameplay objects as players: the pawn's equipped AFortWeapon and
// its gameplay abilities. How to *pull the trigger* from the server differs by what a build exposes,
// so every mechanism is detected by name at runtime and tried in order:
//
//   Fire:   AFortPawn::PawnStartFire / PawnStopFire (the pawn-level fire API AI uses)
//           -> UAbilitySystemComponent::TryActivateAbility / CancelAbilityHandle on the weapon's
//              PrimaryAbilitySpecHandle (how the weapon's own fire ability is started)
//   Reload: TryActivateAbility on the weapon's ReloadAbilitySpecHandle
//
// If none exists on a build, IsFiringSupported() is false and this is logged once; combat then only
// positions and aims, it never fakes damage.
namespace BotWeapons
{
    AFortWeapon* GetCurrentWeapon(AFortPlayerPawnAthena* Pawn);
    const UFortItemDefinition* GetWeaponData(AFortWeapon* Weapon);

    // Rounds in the weapon's magazine (AFortWeapon::AmmoCount). -1 if the build does not expose it.
    int GetLoadedAmmo(AFortWeapon* Weapon);
    void SetLoadedAmmo(AFortWeapon* Weapon, int Ammo);

    bool StartFire(FBotController& Bot);
    void StopFire(FBotController& Bot);
    bool StartReload(FBotController& Bot);

    bool IsFiringSupported();

    // Seconds a reload takes, from the weapon stats (ReloadTime), default 1.5 s.
    float GetReloadTime(const UFortItemDefinition* Weapon);
}
