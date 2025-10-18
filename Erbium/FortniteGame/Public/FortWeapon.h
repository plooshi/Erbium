#pragma once
#include "../../pch.h"
#include "FortInventory.h"
#include "../../Engine/Public/AbilitySystemComponent.h"

class AFortWeapon : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortWeapon);

    DEFINE_PROP(ItemEntryGuid, FGuid);
    DEFINE_PROP(WeaponData, UFortWeaponItemDefinition*);
    DEFINE_PROP(ContextTrapItemDefinition, UFortItemDefinition*);
    DEFINE_PROP(PrimaryAbilitySpecHandle, FGameplayAbilitySpecHandle);
    DEFINE_PROP(SecondaryAbilitySpecHandle, FGameplayAbilitySpecHandle);
    DEFINE_PROP(ReloadAbilitySpecHandle, FGameplayAbilitySpecHandle);
    DEFINE_PROP(ImpactAbilitySpecHandle, FGameplayAbilitySpecHandle);
    DEFINE_PROP(ReticleTraceOverrideSpecHandle, FGameplayAbilitySpecHandle);
    DEFINE_PROP(EquippedAbilityHandles, TArray<FGameplayAbilitySpecHandle>);

    DEFINE_FUNC(ServerReleaseWeaponAbility, void);
};