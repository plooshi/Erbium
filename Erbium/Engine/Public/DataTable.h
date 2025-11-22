#pragma once
#include "../../pch.h"

class UDataTable : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UDataTable);

    TMap<FName, uint8_t*>& GetRowMap() const
    {
        return *(TMap<FName, uint8*>*)(__int64(this) + 0x30);
    }
    __declspec(property(get = GetRowMap))
    TMap<FName, uint8_t*> RowMap;
};

class UCompositeDataTable : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UCompositeDataTable);
    
    DEFINE_PROP(ParentTables, TArray<UDataTable*>);
};

struct FDataTableRowHandle
{
public:
    UDataTable* DataTable;
    FName RowName;
};

struct FDataTableCategoryHandle
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FDataTableCategoryHandle);

    DEFINE_STRUCT_PROP(DataTable, UDataTable*);
    DEFINE_STRUCT_PROP(ColumnName, FName);
    DEFINE_STRUCT_PROP(RowContents, FName);
};

struct alignas(0x08) FTableRowBase
{
public:
};