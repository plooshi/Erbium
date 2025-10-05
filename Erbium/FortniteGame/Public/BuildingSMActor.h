#pragma once
#include "../../pch.h"
#include "FortPlayerControllerAthena.h"
#include "GameplayTagContainer.h"

class EFortResourceType
{
public:
    UENUM_COMMON_MEMBERS(EFortResourceType);

    DEFINE_ENUM_PROP(Wood);
    DEFINE_ENUM_PROP(Stone);
    DEFINE_ENUM_PROP(Metal);
    DEFINE_ENUM_PROP(Permanite);
    DEFINE_ENUM_PROP(GoldCurrency);
    DEFINE_ENUM_PROP(Ingredient);
    DEFINE_ENUM_PROP(None);
};

class ABuildingSMActor : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingSMActor);

    DEFINE_BITFIELD_PROP(bPlayerPlaced);
    DEFINE_PROP(BuildingResourceAmountOverride, FCurveTableRowHandle);
    DEFINE_PROP(ResourceType, uint8_t);
    DEFINE_PROP(Team, uint8);
    DEFINE_PROP(TeamIndex, uint8);
    DEFINE_PROP(EditingPlayer, AFortPlayerStateAthena*);
    DEFINE_BITFIELD_PROP(bDestroyed);
    DEFINE_PROP(CurrentBuildingLevel, int32);
    DEFINE_BITFIELD_PROP(bAllowResourceDrop);

    DEFINE_FUNC(GetHealth, float);
    DEFINE_FUNC(GetMaxHealth, float);
    DEFINE_FUNC(SetMirrored, void);
    DEFINE_FUNC(InitializeKismetSpawnedBuildingActor, void);
    DEFINE_FUNC(GetHealthPercent, float);
    DEFINE_FUNC(RepairBuilding, void);
    
    DefHookOg(void, OnDamageServer, ABuildingSMActor*, float, FGameplayTagContainer, FVector, __int64, AFortPlayerControllerAthena*, AActor*, __int64);

    InitHooks;
};