#pragma once
#include "../../pch.h"
#include "FortPlayerControllerAthena.h"
#include "GameplayTagContainer.h"

enum class EFortResourceType : uint8
{
    Wood = 0,
    Stone = 1,
    Metal = 2,
    Permanite = 3,
    GoldCurrency = 4,
    None = 5
};

class ABuildingSMActor : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingSMActor);

    DEFINE_BITFIELD_PROP(bPlayerPlaced);
    DEFINE_PROP(BuildingResourceAmountOverride, FCurveTableRowHandle);
    DEFINE_PROP(ResourceType, EFortResourceType);
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