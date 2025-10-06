#pragma once
#include "../../pch.h"
#include "FortPlaylistAthena.h"

struct FCollectorUnitInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FCollectorUnitInfo);

    DEFINE_STRUCT_PROP(OutputItemEntry, TArray<FFortItemEntry>);
    DEFINE_STRUCT_PROP(OutputItem, const UFortItemDefinition*);
    DEFINE_STRUCT_PROP(InputCount, FScalableFloat);
};

class ABuildingItemCollectorActor : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingItemCollectorActor);

    DEFINE_PROP(ItemCollections, TArray<FCollectorUnitInfo>);
    DEFINE_PROP(StartingGoalLevel, int32);
};