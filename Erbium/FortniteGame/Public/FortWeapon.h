#pragma once
#include "../../pch.h"
#include "FortInventory.h"
#include "../../Engine/Public/AbilitySystemComponent.h"

class AFortWeapon : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortWeapon);

    DEFINE_PROP(ItemEntryGuid, FGuid);
    DEFINE_PROP(WeaponData, UFortItemDefinition*);
    DEFINE_PROP(ContextTrapItemDefinition, UFortItemDefinition*);
    DEFINE_PROP(PrimaryAbilitySpecHandle, FGameplayAbilitySpecHandle);

    DEFINE_FUNC(ServerReleaseWeaponAbility, void);
};