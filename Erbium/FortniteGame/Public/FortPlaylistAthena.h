#pragma once
#include "../../pch.h"
#include "../../Engine/Public/DataTable.h"
#include "../../Engine/Public/DataTableFunctionLibrary.h"
#include "../../Engine/Public/CurveTable.h"

struct FScalableFloat
{
public:
    float Value;
    uint8 _Padding[0x4];
    FCurveTableRowHandle Curve;

    inline float Evaluate()
    {
        if (!Curve.CurveTable)
            return Value;

        float Out;
        UDataTableFunctionLibrary::EvaluateCurveTableRow(Curve.CurveTable, Curve.RowName, (float)0, nullptr, &Out, FString());
        return Out;
    }
};

class UFortPlaylistAthena : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortPlaylistAthena);

    DEFINE_PROP(PlaylistName, FName);
    DEFINE_PROP(PlaylistId, int32);
    DEFINE_PROP(MaxPlayers, int32);
    DEFINE_PROP(AdditionalLevels, TArray<TSoftObjectPtr<UWorld>>);
    DEFINE_PROP(AdditionalLevelsServerOnly, TArray<TSoftObjectPtr<UWorld>>);
    DEFINE_PROP(GarbageCollectionFrequency, float);
    DEFINE_PROP(MaxSquadSize, int32);
    DEFINE_PROP(LootTierData, TSoftObjectPtr<UDataTable>);
    DEFINE_PROP(LootPackages, TSoftObjectPtr<UDataTable>);
    DEFINE_PROP(AirCraftBehavior, uint8);
    DEFINE_PROP(SafeZoneStartUp, uint8);
    DEFINE_PROP(bRespawnInAir, bool);
    DEFINE_PROP(RespawnHeight, FScalableFloat);
    DEFINE_PROP(RespawnTime, FScalableFloat);
    DEFINE_PROP(RespawnType, uint8);
    DEFINE_PROP(bAllowJoinInProgress, bool);
    DEFINE_PROP(GameData, TSoftObjectPtr<UCurveTable>);
};