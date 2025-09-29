#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"
#include "FortPlayerPawnAthena.h"
#include "../../Engine/Public/AbilitySystemComponent.h"
#include "../../Engine/Public/DataTable.h"
#include "../../Engine/Public/DataTableFunctionLibrary.h"
#include "../../Engine/Public/CurveTable.h"

struct FGuid
{
    int32 A;
    int32 B;
    int32 C;
    int32 D;

    bool operator==(FGuid& _Rhs)
    {
        return A == _Rhs.A && B == _Rhs.B && C == _Rhs.C && D == _Rhs.D;
    }
};


class UFortItem : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortItem);
};

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

class UFortItemDefinition : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortItemDefinition);

    DEFINE_BITFIELD_PROP(bForceIntoOverflow);

    DEFINE_FUNC(CreateTemporaryItemInstanceBP, UFortItem*);
    
    int32 GetMaxStackSize()
    {
        static auto Prop = this->GetProperty("MaxStackSize");
        static auto ElementSizeOff = VersionInfo.EngineVersion >= 4.25 && VersionInfo.FortniteVersion < 20 ? 0x3c : (VersionInfo.EngineVersion >= 5.2 ? 0x2c : 0x34);
        static auto OffsetOff = VersionInfo.EngineVersion >= 4.25 && VersionInfo.FortniteVersion < 20 ? 0x4c : (VersionInfo.EngineVersion >= 5.2 ? 0x3c : 0x44);

        static auto MaxStackSizeSize = GetFromOffset<int32>(Prop, ElementSizeOff); // tuff variable name
        static auto MaxStackSizeOffset = GetFromOffset<int32>(Prop, OffsetOff);

        if (MaxStackSizeSize == 4) // sizeof(int32)
            return GetFromOffset<int32>(this, MaxStackSizeOffset);

        // scalablefloat
        auto& ScalableFloat = GetFromOffset<FScalableFloat>(this, MaxStackSizeOffset);
        return (int32)ScalableFloat.Evaluate();
    }
};

struct FFortItemEntry : public FFastArraySerializerItem
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortItemEntry);

    DEFINE_STRUCT_PROP(LoadedAmmo, int32);
    DEFINE_STRUCT_PROP(PhantomReserveAmmo, int32);
    DEFINE_STRUCT_PROP(ItemGuid, FGuid);
    DEFINE_STRUCT_PROP(TrackerGuid, FGuid);
    DEFINE_STRUCT_PROP(ItemDefinition, UFortItemDefinition*);
    DEFINE_STRUCT_PROP(Count, int32);
    DEFINE_STRUCT_PROP(Durability, float);
    DEFINE_STRUCT_PROP(GameplayAbilitySpecHandle, FGameplayAbilitySpecHandle);
    DEFINE_STRUCT_PROP(ParentInventory, TWeakObjectPtr<class AFortInventory>);
    DEFINE_STRUCT_PROP(Level, int32);
};

class UFortWorldItem : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortWorldItem);

    DEFINE_PROP(ItemEntry, FFortItemEntry);

    DEFINE_FUNC(SetOwningControllerForTemporaryItem, void);
};

struct FFortItemList : public FFastArraySerializer
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortItemList);

    DEFINE_STRUCT_PROP(ReplicatedEntries, TArray<FFortItemEntry>);
    DEFINE_STRUCT_PROP(ItemInstances, TArray<UFortWorldItem*>);
};

struct FItemAndCount
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FItemAndCount);

    DEFINE_STRUCT_PROP(Count, int32);
    DEFINE_STRUCT_PROP(Item, UFortItemDefinition*);
};

struct EFortPickupSourceTypeFlag
{
public:
    UENUM_COMMON_MEMBERS(EFortPickupSourceTypeFlag);

    DEFINE_ENUM_PROP(Other);
    DEFINE_ENUM_PROP(Player);
    DEFINE_ENUM_PROP(Destruction);
    DEFINE_ENUM_PROP(Container);
    DEFINE_ENUM_PROP(AI);
    DEFINE_ENUM_PROP(Tossed);
    DEFINE_ENUM_PROP(FloorLoot);
    DEFINE_ENUM_PROP(Fishing);
};

struct EFortPickupSpawnSource
{
public:
    UENUM_COMMON_MEMBERS(EFortPickupSpawnSource);

    DEFINE_ENUM_PROP(Unset);
    DEFINE_ENUM_PROP(PlayerElimination);
    DEFINE_ENUM_PROP(Chest);
    DEFINE_ENUM_PROP(SupplyDrop);
    DEFINE_ENUM_PROP(AmmoBox);
    DEFINE_ENUM_PROP(Drone);
    DEFINE_ENUM_PROP(ItemSpawner);
    DEFINE_ENUM_PROP(BotElimination);
    DEFINE_ENUM_PROP(NPCElimination);
    DEFINE_ENUM_PROP(LootDrop);
    DEFINE_ENUM_PROP(TossedByPlayer);
};

class AFortPickupAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPickupAthena);

    DEFINE_PROP(bRandomRotation, bool);
    DEFINE_PROP(PrimaryPickupItemEntry, FFortItemEntry);
    DEFINE_PROP(PawnWhoDroppedPickup, AFortPlayerPawnAthena*);
    DEFINE_PROP(bTossedFromContainer, bool);

    DEFINE_FUNC(OnRep_PrimaryPickupItemEntry, bool);
    DEFINE_FUNC(OnRep_TossedFromContainer, bool);
    DEFINE_FUNC(TossPickup, void);
};

class UFortWeaponItemDefinition : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortWeaponItemDefinition);

    DEFINE_BITFIELD_PROP(bUsesPhantomReserveAmmo);
    DEFINE_PROP(WeaponStatHandle, FDataTableRowHandle);
};

struct FFortRangedWeaponStats
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortRangedWeaponStats);

    DEFINE_STRUCT_PROP(ClipSize, int32);
    DEFINE_STRUCT_PROP(InitialClips, int32);
};

class AFortInventory : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortInventory);

    DEFINE_PROP(Inventory, FFortItemList);
    DEFINE_PROP(bRequiresLocalUpdate, bool);
    
    DEFINE_FUNC(HandleInventoryLocalUpdate, void);

    UFortWorldItem* GiveItem(UFortItemDefinition*, int = 1, int = 0, int = 0, bool = true, bool = true, int = 0);
    UFortWorldItem* GiveItem(FFortItemEntry, int = -1, bool = true, bool = true);
    void Update(FFortItemEntry*);
    void Remove(FGuid);
    static AFortPickupAthena* SpawnPickup(FVector, FFortItemEntry&, long long = EFortPickupSourceTypeFlag::GetTossed(), long long = EFortPickupSpawnSource::GetUnset(), AFortPlayerPawnAthena* = nullptr, int = -1, bool = true, bool = true, bool = true);
    static AFortPickupAthena* SpawnPickup(FVector, UFortItemDefinition*, int, int, long long = EFortPickupSourceTypeFlag::GetTossed(), long long = EFortPickupSpawnSource::GetUnset(), AFortPlayerPawnAthena* = nullptr, bool = true, bool = true);
    static FFortItemEntry* MakeItemEntry(UFortItemDefinition*, int32, int32);
    static FFortRangedWeaponStats* GetStats(UFortWeaponItemDefinition*);
    static bool IsPrimaryQuickbar(UFortItemDefinition*);
    void UpdateEntry(FFortItemEntry&);
};