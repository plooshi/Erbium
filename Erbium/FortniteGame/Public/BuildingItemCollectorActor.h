#pragma once
#include "../../pch.h"
#include "FortPlaylistAthena.h"
#include "FortPlayerControllerAthena.h"

struct FCollectorUnitInfo
{
public:
    static const SDK::UStruct* StaticStruct()
    {
        static const SDK::UStruct* _storage = nullptr;

        if (!_storage)
            _storage = SDK::FindStruct("CollectorUnitInfo");

        if (!_storage)
            _storage = SDK::FindStruct("ColletorUnitInfo");

        return _storage;
    }

    static const int32 Size()
    {
        static int32 _size = -1;

        if (_size == -1)
            _size = StaticStruct()->GetPropertiesSize();

        return _size;
    }

    FCollectorUnitInfo& operator=(FCollectorUnitInfo& _Rhs)
    {
        memcpy((PBYTE)this, (const PBYTE)&_Rhs, Size());
        return *this;
    }

    DEFINE_STRUCT_PROP(OutputItemEntry, TArray<FFortItemEntry>);
    DEFINE_STRUCT_PROP(OutputItem, const UFortItemDefinition*);
    DEFINE_STRUCT_PROP(InputItem, const UFortItemDefinition*);
    DEFINE_STRUCT_PROP(InputCount, FScalableFloat);
    DEFINE_STRUCT_PROP(bUseDefinedOutputItem, bool);
};

class ABuildingItemCollectorActor : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingItemCollectorActor);

    DEFINE_PROP(ItemCollections, TArray<FCollectorUnitInfo>);
    DEFINE_PROP(StartingGoalLevel, int32);
    DEFINE_PROP(ActiveInputItem, UFortItemDefinition*);
    DEFINE_PROP(ClientPausedActiveInputItem, UFortItemDefinition*);
    DEFINE_PROP(LootSpawnLocation, FVector);
    DEFINE_BITFIELD_PROP(bCurrentInteractionSuccess);
    DEFINE_PROP(ControllingPlayer, AFortPlayerControllerAthena*);
    DEFINE_PROP(PickupSpawned, TMulticastInlineDelegate<void()>);
    DEFINE_PROP(DefaultItemLootTierGroupName, FName);

    DEFINE_FUNC(PlayVendFailFX, void);
    DEFINE_FUNC(PlayVendFX, void);
    DEFINE_FUNC(DoVendDeath, void);
};