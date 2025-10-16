#pragma once
#include "../../pch.h"
#include "FortPlayerPawnAthena.h"
#include "../../Engine/Public/AbilitySystemComponent.h"
#include "../../Engine/Public/DataTable.h"
#include "../../Engine/Public/DataTableFunctionLibrary.h"
#include "../../Engine/Public/CurveTable.h"
#include "BuildingContainer.h"
#include "FortPlaylistAthena.h"

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

class UFortItemDefinition : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortItemDefinition);

    DEFINE_BITFIELD_PROP(bForceIntoOverflow);
    DEFINE_PROP(MinLevel, int32);
    DEFINE_PROP(MaxLevel, int32);
    DEFINE_PROP(DropCount, int32);
    DEFINE_PROP(NumberOfSlotsToTake, uint8);
    DEFINE_BITFIELD_PROP(bAllowMultipleStacks);
    DEFINE_PROP(LootLevelData, FDataTableCategoryHandle);
    DEFINE_PROP(Tier, uint8);
    DEFINE_BITFIELD_PROP(bInventorySizeLimited);
    DEFINE_BITFIELD_PROP(bForceFocusWhenAdded);

    DEFINE_FUNC(CreateTemporaryItemInstanceBP, UFortItem*);
    DEFINE_FUNC(GetWeaponItemDefinition, UFortItemDefinition*);
    
    int32 GetMaxStackSize() const
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

struct FFortItemEntryStateValue
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortItemEntryStateValue);

    DEFINE_STRUCT_PROP(IntValue, int);
    DEFINE_STRUCT_PROP(NameValue, FName);
    DEFINE_STRUCT_PROP(StateType, uint8_t);
};

struct FFortItemEntry : public FFastArraySerializerItem
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortItemEntry);

    DEFINE_STRUCT_PROP(LoadedAmmo, int32);
    DEFINE_STRUCT_PROP(PhantomReserveAmmo, int32);
    DEFINE_STRUCT_PROP(ItemGuid, FGuid);
    DEFINE_STRUCT_PROP(TrackerGuid, FGuid);
    DEFINE_STRUCT_PROP(ItemDefinition, const UFortItemDefinition*);
    DEFINE_STRUCT_PROP(Count, int32);
    DEFINE_STRUCT_PROP(Durability, float);
    DEFINE_STRUCT_PROP(GameplayAbilitySpecHandle, FGameplayAbilitySpecHandle);
    DEFINE_STRUCT_PROP(ParentInventory, TWeakObjectPtr<class AFortInventory>);
    DEFINE_STRUCT_PROP(Level, int32);
    DEFINE_STRUCT_PROP(StateValues, TArray<FFortItemEntryStateValue>);
    DEFINE_STRUCT_PROP(bIsReplicatedCopy, bool);
    DEFINE_STRUCT_PROP(bIsDirty, bool);
};

class UFortWorldItem : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortWorldItem);

    DEFINE_PROP(ItemEntry, FFortItemEntry);
    DEFINE_PROP(OwnerInventory, AActor*);

    DEFINE_FUNC(SetOwningControllerForTemporaryItem, void);
    DEFINE_FUNC(GetOwningController, AActor*);
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

struct FFortPickupLocationData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortPickupLocationData);

    DEFINE_STRUCT_PROP(bPlayPickupSound, bool);
    DEFINE_STRUCT_PROP(FlyTime, float);
    DEFINE_STRUCT_NEWOBJ_PROP(ItemOwner, AFortPlayerPawnAthena);
    DEFINE_STRUCT_PROP(PickupGuid, FGuid);
    DEFINE_STRUCT_NEWOBJ_PROP(PickupTarget, AFortPlayerPawnAthena);
};

class AFortPickupAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPickupAthena);

    DEFINE_PROP(bRandomRotation, bool);
    DEFINE_PROP(PrimaryPickupItemEntry, FFortItemEntry);
    DEFINE_NEWOBJ_PROP(PawnWhoDroppedPickup, AFortPlayerPawnAthena);
    DEFINE_PROP(bTossedFromContainer, bool);
    DEFINE_PROP(bPickedUp, bool);
    DEFINE_PROP(PickupLocationData, FFortPickupLocationData);
    DEFINE_PROP(MovementComponent, UObject*);

    DEFINE_FUNC(OnRep_PrimaryPickupItemEntry, void);
    DEFINE_FUNC(OnRep_TossedFromContainer, void);
    DEFINE_FUNC(TossPickup, void);
    DEFINE_FUNC(OnRep_bPickedUp, void);
    DEFINE_FUNC(OnRep_PickupLocationData, void);
};

class UFortWeaponItemDefinition : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortWeaponItemDefinition);

    DEFINE_BITFIELD_PROP(bUsesPhantomReserveAmmo);
    DEFINE_PROP(WeaponStatHandle, FDataTableRowHandle);
    
    DEFINE_FUNC(GetAmmoWorldItemDefinition_BP, UFortItemDefinition*);
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
    DEFINE_PROP(bRequiresSaving, bool);
    DEFINE_PROP(InventoryType, uint8_t);
    
    DEFINE_FUNC(HandleInventoryLocalUpdate, void);

    UFortWorldItem* GiveItem(const UFortItemDefinition*, int = 1, int = 0, int = 0, bool = true, bool = true, int = 0, TArray<FFortItemEntryStateValue> = {});
    UFortWorldItem* GiveItem(FFortItemEntry&, int = -1, bool = true, bool = true);
    void Update(FFortItemEntry*);
    void Remove(FGuid);
    static AFortPickupAthena* SpawnPickup(FVector, FFortItemEntry&, long long = EFortPickupSourceTypeFlag::GetOther(), long long = EFortPickupSpawnSource::GetUnset(), AFortPlayerPawnAthena* = nullptr, int = -1, bool = true, bool = true, bool = true);
    static AFortPickupAthena* SpawnPickup(FVector, const UFortItemDefinition*, int, int, long long = EFortPickupSourceTypeFlag::GetOther(), long long = EFortPickupSpawnSource::GetUnset(), AFortPlayerPawnAthena* = nullptr, bool = true, bool = true);
    static AFortPickupAthena* SpawnPickup(ABuildingContainer*, FFortItemEntry&, AFortPlayerPawnAthena* = nullptr, int = -1);
    static FFortItemEntry* MakeItemEntry(const UFortItemDefinition*, int32, int32);
    static FFortRangedWeaponStats* GetStats(UFortWeaponItemDefinition*);
    static bool IsPrimaryQuickbar(const UFortItemDefinition*);
    void UpdateEntry(FFortItemEntry&);
    void SetRequiresUpdate();

    InitHooks;
};