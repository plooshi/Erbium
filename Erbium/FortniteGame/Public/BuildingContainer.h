#pragma once
#include "pch.h"

struct FFortSearchBounceData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortSearchBounceData);

    DEFINE_STRUCT_PROP(SearchAnimationCount, uint32);
};


class ABuildingContainer : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingContainer);

    DEFINE_PROP(SearchLootTierGroup, FName);
    DEFINE_PROP(LootSpawnLocation_Athena, FVector);
    DEFINE_BITFIELD_PROP(bAlreadySearched);
    DEFINE_PROP(SearchBounceData, FFortSearchBounceData);

    DEFINE_FUNC(OnRep_bAlreadySearched, void);
    DEFINE_FUNC(BounceContainer, void);
};