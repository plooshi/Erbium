#pragma once
#include "pch.h"
#include "Utils.h"

class UDataTable : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UDataTable);

    DEFINE_PROP(RowMap, TMap<FName, uint8*>);
};