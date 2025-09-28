#pragma once
#include "pch.h"
#include "Utils.h"

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

    DEFINE_FUNC(CreateTemporaryItemInstanceBP, UFortItem*);
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
};