#pragma once
#include "../../pch.h"
#include "FortPlaylistAthena.h"

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
        __movsb((PBYTE)this, (const PBYTE)&_Rhs, Size());             
        return *this;                                                 
    }    

    DEFINE_STRUCT_PROP(OutputItemEntry, TArray<FFortItemEntry>);
    DEFINE_STRUCT_PROP(OutputItem, const UFortItemDefinition*);
    DEFINE_STRUCT_PROP(InputCount, FScalableFloat);
};

struct FColletorUnitInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FColletorUnitInfo);

    DEFINE_STRUCT_PROP(OutputItemEntry, TArray<FFortItemEntry>);
    DEFINE_STRUCT_PROP(OutputItem, const UFortItemDefinition*);
    DEFINE_STRUCT_PROP(InputCount, FScalableFloat);
};

class ABuildingItemCollectorActor : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingItemCollectorActor);

    DEFINE_PROP(ItemCollections, TArray<FCollectorUnitInfo>);
    DEFINE_PROP(StartingGoalLevel, int32);
};