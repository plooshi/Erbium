#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"
#include "FortPlayerControllerAthena.h"
#include "GameplayTagContainer.h"

enum EFortResourceType : uint8_t
{
    Wood = 0,
    Stone = 1,
    Metal = 2,
    Permanite = 3,
    None = 4
};

class ABuildingSMActor : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingSMActor);

    DEFINE_BITFIELD_PROP(bPlayerPlaced);
    DEFINE_PROP(BuildingResourceAmountOverride, FCurveTableRowHandle);
    DEFINE_PROP(ResourceType, EFortResourceType);

    DEFINE_FUNC(GetHealth, float);
    DEFINE_FUNC(GetMaxHealth, float);
    
    DefHookOg(void, OnDamageServer, ABuildingSMActor*, float, FGameplayTagContainer, FVector, __int64, AFortPlayerControllerAthena*, AActor*, __int64);

    InitHooks;
};