#pragma once
#include "pch.h"
#include "../../Erbium/Public/Utils.h"
#include "FortInventory.h"
#include <numeric>
#include "FortGameStateAthena.h"
#include "../../Engine/Public/DataTable.h"

struct FFortGameFeatureLootTableData
{
    TSoftObjectPtr<UDataTable> LootTierData;
    TSoftObjectPtr<UDataTable> LootPackageData;
};

struct FFortLootPackageData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortLootPackageData);

    DEFINE_STRUCT_PROP(LootPackageID, FName);
    DEFINE_STRUCT_PROP(Weight, float);
    DEFINE_STRUCT_PROP(LootPackageCall, FString);
    DEFINE_STRUCT_PROP(ItemDefinition, TSoftObjectPtr<UFortItemDefinition>);
    DEFINE_STRUCT_PROP(Count, int32);
    DEFINE_STRUCT_PROP(LootPackageCategory, int);
    DEFINE_STRUCT_PROP(MinWorldLevel, int);
    DEFINE_STRUCT_PROP(MaxWorldLevel, int);
};


struct FFortLootTierData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortLootTierData);

    DEFINE_STRUCT_PROP(NumLootPackageDrops, float);
    DEFINE_STRUCT_PROP(TierGroup, FName);
    DEFINE_STRUCT_PROP(LootTier, int);
    DEFINE_STRUCT_PROP(Weight, float);
    DEFINE_STRUCT_PROP(LootPackage, FName);
    DEFINE_STRUCT_PROP(LootPackageCategoryWeightArray, TArray<int32>);
    DEFINE_STRUCT_PROP(LootPackageCategoryMinArray, TArray<int32>);
    DEFINE_STRUCT_PROP(LootPackageCategoryMaxArray, TArray<int32>);
};


inline TArray<FFortLootTierData*> TierDataAllGroups;
inline TArray<FFortLootPackageData*> LPGroupsAll;

template <typename T>
static T* PickWeighted(TArray<T*>& Map, float (*RandFunc)(float), bool bCheckZero = true) {
    float TotalWeight = std::accumulate(Map.begin(), Map.end(), 0.0f, [&](float acc, T*& p) { return acc + p->Weight; });
    float RandomNumber = RandFunc(TotalWeight);

    for (auto& Element : Map)
    {
        float Weight = Element->Weight;
        if (bCheckZero && Weight == 0)
            continue;

        if (RandomNumber <= Weight) return Element;

        RandomNumber -= Weight;
    }

    return nullptr;
}

class UFortLootPackage
{
public:
    static void SetupLDSForPackage(TArray<FFortItemEntry*>&, SDK::FName, int, FName, int WorldLevel = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->WorldLevel);
    static TArray<FFortItemEntry*> ChooseLootForContainer(FName, int = -1, int = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->WorldLevel);

    InitHooks;
};