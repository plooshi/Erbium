#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"

struct FDynamicBuildingFoundationRepData final
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FDynamicBuildingFoundationRepData);

    DEFINE_STRUCT_PROP(Rotation, FRotator);
    DEFINE_STRUCT_PROP(Translation, FVector);
    DEFINE_STRUCT_PROP(EnabledState, uint8);
};

struct FBuildingFoundationStreamingData final
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FBuildingFoundationStreamingData);

    DEFINE_STRUCT_PROP(FoundationLocation, FVector);
};

class ABuildingFoundation : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingFoundation);

    DEFINE_PROP(FoundationEnabledState, uint8);
    DEFINE_PROP(DynamicFoundationRepData, FDynamicBuildingFoundationRepData);
    DEFINE_PROP(DynamicFoundationTransform, FTransform);
    DEFINE_PROP(StreamingData, FBuildingFoundationStreamingData);

    DEFINE_FUNC(OnRep_DynamicFoundationRepData, void);

    static void SetDynamicFoundationEnabled(UObject*, FFrame&);
    static void SetDynamicFoundationTransform_(UObject*, FFrame&);

    InitHooks;
};