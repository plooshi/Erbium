#include "pch.h"
#include "../../Public/Combat/BotWeapons.h"
#include "../../Public/BotController.h"
#include "../../Public/BotLog.h"
#include "../../../FortniteGame/Public/FortWeapon.h"

enum class EFireMethod : uint8
{
    Unknown,
    PawnFire,   // PawnStartFire / PawnStopFire
    Ability,    // TryActivateAbility / CancelAbilityHandle
    Unsupported,
};

static EFireMethod FireMethod = EFireMethod::Unknown;

static EFireMethod DetectFireMethod(AFortPlayerPawnAthena* Pawn, UAbilitySystemComponent* AbilitySystem)
{
    if (FireMethod != EFireMethod::Unknown)
        return FireMethod;

    if (Pawn->GetFunction("PawnStartFire") && Pawn->GetFunction("PawnStopFire"))
        FireMethod = EFireMethod::PawnFire;
    else if (AbilitySystem && AbilitySystem->GetFunction("TryActivateAbility") && AbilitySystem->GetFunction("CancelAbilityHandle"))
        FireMethod = EFireMethod::Ability;
    else
        FireMethod = EFireMethod::Unsupported;

    static const char* Names[] = { "unknown", "PawnStartFire", "weapon ability activation", "none" };
    BOT_VERBOSE(BOT_CATEGORY_COMBAT, "Firing method on this build: %s.", Names[(int)FireMethod]);
    return FireMethod;
}

static UAbilitySystemComponent* GetAbilitySystem(FBotController& Bot)
{
    auto PlayerState = Bot.GetPlayerState();
    return PlayerState ? PlayerState->AbilitySystemComponent : nullptr;
}

static bool TryActivateAbility(UAbilitySystemComponent* AbilitySystem, FGameplayAbilitySpecHandle Handle)
{
    static auto Func = AbilitySystem->GetFunction("TryActivateAbility");
    if (!Func || Handle.Handle == -1)
        return false;

    bool bAllowRemoteActivation = true;
    return AbilitySystem->Call<bool>(Func, Handle, bAllowRemoteActivation);
}

namespace BotWeapons
{
    AFortWeapon* GetCurrentWeapon(AFortPlayerPawnAthena* Pawn)
    {
        if (!Pawn)
            return nullptr;

        auto Weapon = (AFortWeapon*)Pawn->CurrentWeapon;
        return Weapon && !Weapon->bActorIsBeingDestroyed ? Weapon : nullptr;
    }

    const UFortItemDefinition* GetWeaponData(AFortWeapon* Weapon)
    {
        return Weapon ? (const UFortItemDefinition*)Weapon->WeaponData : nullptr;
    }

    int GetLoadedAmmo(AFortWeapon* Weapon)
    {
        if (!Weapon)
            return -1;

        static auto Offset = Weapon->GetOffset("AmmoCount");
        return Offset != -1 ? *(int32*)(__int64(Weapon) + Offset) : -1;
    }

    void SetLoadedAmmo(AFortWeapon* Weapon, int Ammo)
    {
        if (!Weapon)
            return;

        static auto Offset = Weapon->GetOffset("AmmoCount");
        static auto OnRep = Weapon->GetFunction("OnRep_AmmoCount");
        if (Offset == -1)
            return;

        *(int32*)(__int64(Weapon) + Offset) = Ammo;
        if (OnRep)
            Weapon->ProcessEvent(OnRep, nullptr);
    }

    bool StartFire(FBotController& Bot)
    {
        auto Pawn = Bot.GetPawn();
        auto Weapon = GetCurrentWeapon(Pawn);
        if (!Weapon)
            return false;

        auto AbilitySystem = GetAbilitySystem(Bot);

        switch (DetectFireMethod(Pawn, AbilitySystem))
        {
        case EFireMethod::PawnFire:
        {
            static auto Func = Pawn->GetFunction("PawnStartFire");
            uint8 FireModeNum = 0;
            Pawn->ProcessEvent(Func, &FireModeNum);
            return true;
        }
        case EFireMethod::Ability:
            return AbilitySystem && TryActivateAbility(AbilitySystem, Weapon->PrimaryAbilitySpecHandle);
        default:
            return false;
        }
    }

    void StopFire(FBotController& Bot)
    {
        auto Pawn = Bot.GetPawn();
        if (!Pawn || FireMethod == EFireMethod::Unknown || FireMethod == EFireMethod::Unsupported)
            return;

        if (FireMethod == EFireMethod::PawnFire)
        {
            static auto Func = Pawn->GetFunction("PawnStopFire");
            uint8 FireModeNum = 0;
            Pawn->ProcessEvent(Func, &FireModeNum);
            return;
        }

        auto Weapon = GetCurrentWeapon(Pawn);
        auto AbilitySystem = GetAbilitySystem(Bot);
        if (!Weapon || !AbilitySystem)
            return;

        static auto Cancel = AbilitySystem->GetFunction("CancelAbilityHandle");
        auto Handle = Weapon->PrimaryAbilitySpecHandle;
        if (Cancel && Handle.Handle != -1)
            AbilitySystem->ProcessEvent(Cancel, &Handle);
    }

    bool StartReload(FBotController& Bot)
    {
        auto Weapon = GetCurrentWeapon(Bot.GetPawn());
        auto AbilitySystem = GetAbilitySystem(Bot);
        if (!Weapon || !AbilitySystem || !Weapon->HasReloadAbilitySpecHandle())
            return false;

        return TryActivateAbility(AbilitySystem, Weapon->ReloadAbilitySpecHandle);
    }

    bool IsFiringSupported()
    {
        return FireMethod != EFireMethod::Unsupported;
    }

    float GetReloadTime(const UFortItemDefinition* Weapon)
    {
        if (!Weapon || !Weapon->IsA<UFortWeaponItemDefinition>())
            return 1.5f;

        auto Stats = AFortInventory::GetStats((const UFortWeaponItemDefinition*)Weapon);
        if (!Stats)
            return 1.5f;

        static auto Offset = FFortRangedWeaponStats::StaticStruct()->GetOffset("ReloadTime");
        if (Offset == -1)
            return 1.5f;

        auto Time = *(float*)(__int64(Stats) + Offset);
        return Time > 0.f && Time < 10.f ? Time : 1.5f;
    }
}
