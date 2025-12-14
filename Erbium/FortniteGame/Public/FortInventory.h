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

class EFortItemType
{
public:
    UENUM_COMMON_MEMBERS(EFortItemType);

    DEFINE_ENUM_PROP(WeaponHarvest);
    DEFINE_ENUM_PROP(WorldResource);
    DEFINE_ENUM_PROP(EditTool);
    DEFINE_ENUM_PROP(Trap);
    DEFINE_ENUM_PROP(Ammo);
    DEFINE_ENUM_PROP(BuildingPiece);
    DEFINE_ENUM_PROP(Ingredient);
};

class EFortRarity
{
public:
    UENUM_COMMON_MEMBERS(EFortRarity);
};

class UFortItemComponent_Pickup : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortItemComponent_Pickup);

    DEFINE_BITFIELD_PROP(bCanBeDroppedFromInventory);
    DEFINE_BITFIELD_PROP(bForceAutoPickup);
};

struct alignas(0x08) FInstancedStruct final
{
public:
    UStruct* ScriptStruct;
    void* Struct;
};

struct FFortItemComponentData_Pickup
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortItemComponentData_Pickup);

    DEFINE_STRUCT_BITFIELD_PROP(bCanBeDroppedFromInventory);
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
    DEFINE_BITFIELD_PROP(bPersistInInventoryWhenFinalStackEmpty);
    DEFINE_BITFIELD_PROP(bCanBeDropped);
    DEFINE_BITFIELD_PROP(bForceAutoPickup);
    DEFINE_PROP(ItemType, uint8);
    DEFINE_PROP(DisplayName, FText);
    DEFINE_PROP(ShortDescription, FText);
    DEFINE_PROP(Description, FText);
    DEFINE_PROP(ItemName, FText);
    DEFINE_PROP(ItemShortDescription, FText);
    DEFINE_PROP(ItemDescription, FText);
    DEFINE_PROP(Rarity, uint8);
    DEFINE_BITFIELD_PROP(bSupportsQuickbarFocus);
    DEFINE_PROP(DataList, TArray<FInstancedStruct>);

    DEFINE_FUNC(CreateTemporaryItemInstanceBP, UFortItem*);
    DEFINE_FUNC(GetItemComponentByClass, UObject*);
    DEFINE_FUNC(GetRichDescription, FText);

    UFortItemComponent_Pickup* GetPickupComponent() const
    {
        return (UFortItemComponent_Pickup*)GetItemComponentByClass(UFortItemComponent_Pickup::StaticClass());
    }

    int32 GetMaxStackSize() const
    {
        static auto Prop = this->GetProperty("MaxStackSize");

        if (Prop)
        {
            static auto MaxStackSizeSize = GetFromOffset<int32>(Prop, Offsets::ElementSize); // tuff variable name
            static auto MaxStackSizeOffset = GetFromOffset<int32>(Prop, Offsets::Offset_Internal);

            if (MaxStackSizeSize == 4) // sizeof(int32)
                return GetFromOffset<int32>(this, MaxStackSizeOffset);

            // scalablefloat
            auto& ScalableFloat = GetFromOffset<FScalableFloat>(this, MaxStackSizeOffset);
            return (int32)ScalableFloat.Evaluate();
        }
        else
        {
            static auto GetMaxStackSizeFn = GetFunction("GetMaxStackSize");
            return Call<int32>(GetMaxStackSizeFn, nullptr);
        }
    }

    bool CanBeDropped() const
    {
        if (HasbCanBeDropped())
            return bCanBeDropped;

        if (HasDataList())
        {
            for (auto& ItemData : DataList)
            {
                if (ItemData.ScriptStruct == FFortItemComponentData_Pickup::StaticStruct())
                {
                    auto PickupComponentData = (FFortItemComponentData_Pickup*)ItemData.Struct;

                    return PickupComponentData->bCanBeDroppedFromInventory;
                }
            }
        }


        auto PickupComponent = GetPickupComponent();

        if (PickupComponent)
            return PickupComponent->bCanBeDroppedFromInventory;
        
        return false;
    }
};

class UFortWorldItemDefinition : public UFortItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortWorldItemDefinition);

    DEFINE_FUNC(GetAmmoWorldItemDefinition_BP, UFortWorldItemDefinition*);
};

struct FFortItemQuantityPair
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortItemQuantityPair);

    DEFINE_STRUCT_PROP(ItemDefinition, TSoftObjectPtr<UFortItemDefinition>);
    DEFINE_STRUCT_PROP(Quantity, int32);
};

class UFortSchematicItemDefinition : public UFortItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortSchematicItemDefinition);

    DEFINE_PROP(CraftingRecipe, FDataTableRowHandle);
    DEFINE_PROP(CraftingRequirements, FDataTableRowHandle);

    DEFINE_FUNC(GetResultWorldItemDefinition, UFortWorldItemDefinition*)
    DEFINE_FUNC(GetQuantityProduced, int32);
};

struct FRecipe : public FTableRowBase
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FRecipe);

    DEFINE_STRUCT_PROP(RecipeResults, TArray<FFortItemQuantityPair>);
    DEFINE_STRUCT_PROP(bIsConsumed, bool);
    DEFINE_STRUCT_PROP(RecipeCosts, TArray<FFortItemQuantityPair>);
    DEFINE_STRUCT_PROP(RequiredCatalysts, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(Score, int32);
};

struct FSchematicRequirement
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FSchematicRequirement);

    DEFINE_STRUCT_PROP(ItemDefinition, UFortItemDefinition*);
    DEFINE_STRUCT_PROP(Count, int32);
};

struct FSchematicRequirements : public FTableRowBase
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FSchematicRequirements);

    DEFINE_STRUCT_PROP(Requirements, TArray<FSchematicRequirement>);
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
    DEFINE_STRUCT_PROP(WeaponModSlots, TArray<void*>);
    DEFINE_STRUCT_PROP(PickupVariantIndex, int32);
    DEFINE_STRUCT_PROP(OrderIndex, int16);
    DEFINE_STRUCT_PROP(ItemVariantDataMappingIndex, int32);
};

class UFortWorldItem : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortWorldItem);

    DEFINE_PROP(ItemEntry, FFortItemEntry);
    DEFINE_PROP(OwnerInventory, AActor*);
    DEFINE_PROP(OwnerInventoryWeak, TWeakObjectPtr<AActor>);

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
    DEFINE_STRUCT_PROP(StartDirection, FVector);
    DEFINE_STRUCT_PROP(LootInitialPosition, FVector);
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

class UFortWeaponItemDefinition : public UFortWorldItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortWeaponItemDefinition);

    DEFINE_BITFIELD_PROP(bUsesPhantomReserveAmmo);
    DEFINE_PROP(WeaponStatHandle, FDataTableRowHandle);
    DEFINE_PROP(PrimaryFireAbility, TSoftClassPtr<UClass>);
    DEFINE_PROP(SecondaryFireAbility, TSoftClassPtr<UClass>);
    DEFINE_PROP(ReloadAbility, TSoftClassPtr<UClass>);
    DEFINE_PROP(OnHitAbility, TSoftClassPtr<UClass>);
    DEFINE_PROP(EquippedAbilities, TArray<TSoftClassPtr<UClass>>);
    DEFINE_PROP(EquippedAbilitySet, TSoftObjectPtr<class UFortAbilitySet>);
    DEFINE_BITFIELD_PROP(bUsesCustomAmmoType);
    DEFINE_PROP(WeaponModSlots, TArray<void*>);
};

struct FFortRangedWeaponStats
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortRangedWeaponStats);

    DEFINE_STRUCT_PROP(ClipSize, int32);
    DEFINE_STRUCT_PROP(InitialClips, int32);
};

class IFortInventoryOwnerInterface : public IInterface
{
public:
    UCLASS_COMMON_MEMBERS(IFortInventoryOwnerInterface);
};

struct FFortWorldMultiItemInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortWorldMultiItemInfo);

    DEFINE_STRUCT_PROP(ItemDefinition, TSoftObjectPtr<UFortItemDefinition>);
    DEFINE_STRUCT_PROP(RequiredXPForNextLevel, FScalableFloat);
};

class UFortWorldMultiItemDefinition : public UFortWorldItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortWorldMultiItemDefinition);

    DEFINE_PROP(ItemInfos, TArray<FFortWorldMultiItemInfo>);
};

class UFortWeaponMeleeItemDefinition : public UFortWeaponItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortWeaponMeleeItemDefinition);
};

class UFortWeaponRangedItemDefinition : public UFortWeaponItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortWeaponRangedItemDefinition);
};

class UFortGadgetItemDefinition : public UFortWorldItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortGadgetItemDefinition);

    DEFINE_FUNC(GetWeaponItemDefinition, UFortWeaponItemDefinition*);
};

class UFortResourceItemDefinition : public UFortWorldItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortResourceItemDefinition);
};

class UFortAmmoItemDefinition : public UFortWorldItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortAmmoItemDefinition);

    DEFINE_PROP(RegenCooldown, FScalableFloat);
};

class UFortDecoItemDefinition : public UFortWorldItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortDecoItemDefinition);

    DEFINE_PROP(BlueprintClass, TSoftClassPtr<UClass>);
    DEFINE_BITFIELD_PROP(bReplacesBuildingWhenPlaced);
    DEFINE_PROP(AutoCreateAttachmentBuildingResourceType, EFortResourceType);
    DEFINE_PROP(AutoCreateAttachmentBuildingShapes, TArray<TSoftObjectPtr<UObject>>);
};

class UFortBuildingItemDefinition : public UFortWorldItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortBuildingItemDefinition);
};

class UFortEditToolItemDefinition : public UFortWorldItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortEditToolItemDefinition);
};


class AFortInventory : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortInventory);

    DEFINE_PROP(Inventory, FFortItemList);
    DEFINE_PROP(bRequiresLocalUpdate, bool);
    DEFINE_PROP(bRequiresSaving, bool);
    DEFINE_PROP(InventoryType, uint8_t);
    DEFINE_PROP(PendingInstances, TArray<UFortWorldItem*>);

    DEFINE_FUNC(HandleInventoryLocalUpdate, void);

    UFortWorldItem* GiveItem(const UFortItemDefinition*, int = 1, int = 0, int = 0, bool = true, bool = true, int = 0, TArray<FFortItemEntryStateValue> = {});
    UFortWorldItem* GiveItem(FFortItemEntry&, int = -1, bool = true, bool = true);
    void Update(FFortItemEntry*);
    void Remove(FGuid);
    static AFortPickupAthena* SpawnPickup(FVector, FFortItemEntry&, long long = EFortPickupSourceTypeFlag::GetOther(), long long = EFortPickupSpawnSource::GetUnset(), AFortPlayerPawnAthena* = nullptr, int = -1, bool = true, bool = true, bool = true, const UClass* = nullptr, FVector = FVector());
    static AFortPickupAthena* SpawnPickup(FVector, const UFortItemDefinition*, int, int, long long = EFortPickupSourceTypeFlag::GetOther(), long long = EFortPickupSpawnSource::GetUnset(), AFortPlayerPawnAthena* = nullptr, bool = true, bool = true, const UClass* = nullptr);
    static AFortPickupAthena* SpawnPickup(ABuildingContainer*, FFortItemEntry&, AFortPlayerPawnAthena* = nullptr, int = -1);
    static FFortItemEntry* MakeItemEntry(const UFortItemDefinition*, int32, int32);
    static FFortRangedWeaponStats* GetStats(const UFortWeaponItemDefinition*);
    static bool IsPrimaryQuickbar(const UFortItemDefinition*);
    void UpdateEntry(FFortItemEntry&);
    void SetRequiresUpdate();
    static void RemoveWeaponAbilities(AActor*);

    InitPostLoadHooks;
};