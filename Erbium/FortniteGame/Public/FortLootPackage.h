#pragma once
#include "../../pch.h"
#include "FortInventory.h"
#include "FortGameStateAthena.h"
#include "../../Engine/Public/DataTable.h"

struct FFortGameFeatureLootTableData
{
public:
    TSoftObjectPtr<UDataTable> LootTierData;
    TSoftObjectPtr<UDataTable> LootPackageData;
};

struct FFortGameFeatureLootTableData_UE52
{
public: 
	static const SDK::UStruct* StaticStruct() 
	{                                         
		static const SDK::UStruct* _storage = nullptr;
        static bool bInitialized = false;     
                                              
		if (!bInitialized)                    
        {                                     
            bInitialized = true;              
	        _storage = SDK::FindStruct("FortGameFeatureLootTableData");
        }                                     
                                              
		return _storage;                      
	}                                         
                                              
	static const int32 Size()                 
	{                                         
		static int32 _size = -1;              
                                              
		if (_size == -1)                      
            _size = StaticStruct()->GetPropertiesSize();  
                                              
		return _size;                         
	}                                         
                                              
    FFortGameFeatureLootTableData_UE52& operator=(FFortGameFeatureLootTableData_UE52& _Rhs)
    {                                         
        __movsb((PBYTE)this, (const PBYTE)&_Rhs, Size()); 
        return *this;                         
    }

    uint8_t Padding[0x40];

    DEFINE_STRUCT_PROP(LootTierData, TSoftObjectPtr<UDataTable>);
    DEFINE_STRUCT_PROP(LootPackageData, TSoftObjectPtr<UDataTable>);
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


//inline TArray<FFortLootTierData*> TierDataAllGroups;
//inline TArray<FFortLootPackageData*> LPGroupsAll;
inline std::map<int32, TArray<FFortLootTierData*>> TierDataMap;
inline std::map<int32, TArray<FFortLootPackageData*>> LootPackageMap;

template <typename T>
static T* PickWeighted(TArray<T*>& Map, float (*RandFunc)(float), bool bCheckZero = true)
    {
    float TotalWeight = std::accumulate(Map.begin(), Map.end(), 0.0f, [&](float acc, T*& p)
    { return acc + p->Weight; });
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

class UBGAConsumableWrapperItemDefinition : public UFortItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UBGAConsumableWrapperItemDefinition);

    DEFINE_PROP(ConsumableClass, TSoftClassPtr<UClass>);
};

class ABGAConsumableSpawner : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABGAConsumableSpawner);

    DEFINE_PROP(SpawnLootTierGroup, FName);
};

class UFortLootPackage
{
public:
    static void SetupLDSForPackage(TArray<FFortItemEntry*>&, SDK::FName, int, FName, int WorldLevel = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->WorldLevel);
    static TArray<FFortItemEntry*> ChooseLootForContainer(FName, int = -1, int = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->WorldLevel);
    static void SpawnFloorLootForContainer(const UClass*);
    static bool SpawnLootHook(ABuildingContainer*);
    static void SpawnLoot(FName&, FVector);
    static void SpawnConsumableActor(ABGAConsumableSpawner*);

    InitHooks;
};