#pragma once
#include "../../pch.h"
#include "BuildingSMActor.h"

struct FFortSearchBounceData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortSearchBounceData);

    DEFINE_STRUCT_PROP(SearchAnimationCount, uint32);
};


struct FVector3f
{
public:
    float X;
    float Y;
    float Z;
};

class ABuildingContainer : public ABuildingSMActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingContainer);

    DEFINE_PROP(SearchLootTierGroup, FName);
    DEFINE_PROP(LootSpawnLocation_Athena, FVector);
    DEFINE_PROP(LootSpawnLocation, FVector);
    DEFINE_BITFIELD_PROP(bAlreadySearched);
    DEFINE_PROP(SearchBounceData, FFortSearchBounceData);
    DEFINE_PROP(LootTossConeHalfAngle_Athena, float);
    DEFINE_PROP(LootTossDirection_Athena, FRotator);
    DEFINE_PROP(LootTossSpeed_Athena, float);
    DEFINE_BITFIELD_PROP(bForceHidePickupMinimapIndicator);
    DEFINE_PROP(ChosenRandomUpgrade, int32);
    DEFINE_PROP(ReplicatedLootTier, int32);
    DEFINE_PROP(LootFinalLocation, FVector);
    DEFINE_BITFIELD_PROP(bDestroyContainerOnSearch);
    DEFINE_BITFIELD_PROP(bStartAlreadySearched_Athena);

    DEFINE_FUNC(OnRep_bAlreadySearched, void);
    DEFINE_FUNC(BounceContainer, void);
    DEFINE_FUNC(OnRep_LootTier, void);
};