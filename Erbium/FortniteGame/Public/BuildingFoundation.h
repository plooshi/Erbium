#pragma once
#include "../../pch.h"

struct FBox
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FBox);
};

struct FDynamicBuildingFoundationRepData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FDynamicBuildingFoundationRepData);

    DEFINE_STRUCT_PROP(Rotation, FRotator);
    DEFINE_STRUCT_PROP(Translation, FVector);
    DEFINE_STRUCT_PROP(EnabledState, uint8);
};

struct FBuildingFoundationStreamingData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FBuildingFoundationStreamingData);

    DEFINE_STRUCT_PROP(FoundationLocation, FVector);
    DEFINE_STRUCT_PROP(BoundingBox, FBox);
};

class ABuildingFoundation : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingFoundation);

    DEFINE_PROP(FoundationEnabledState, uint8);
    DEFINE_PROP(DynamicFoundationRepData, FDynamicBuildingFoundationRepData);
    DEFINE_PROP(DynamicFoundationTransform, FTransform);
    DEFINE_PROP(StreamingData, FBuildingFoundationStreamingData);
    DEFINE_PROP(StreamingBoundingBox, FBox);
    DEFINE_PROP(DynamicFoundationType, uint8);
    DEFINE_BITFIELD_PROP(bServerStreamedInLevel);

    DEFINE_FUNC(OnRep_DynamicFoundationRepData, void);
    //DEFINE_FUNC(SetDynamicFoundationTransform, void);
    DEFINE_FUNC(SetDynamicFoundationEnabled, void);
    DEFINE_FUNC(OnRep_ServerStreamedInLevel, void);
    DEFINE_FUNC(OnRep_LevelToStream, void);

    static void SetDynamicFoundationEnabled_(UObject*, FFrame&);
    static void SetDynamicFoundationTransform_(UObject*, FFrame&);

    InitHooks;
};