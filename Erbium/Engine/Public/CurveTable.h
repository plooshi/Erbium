#pragma once
#include "../../pch.h"

struct FRealCurve
{

};

struct FSimpleCurveKey
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FSimpleCurveKey);

    DEFINE_STRUCT_PROP(Time, float);
    DEFINE_STRUCT_PROP(Value, float);
};

struct FSimpleCurve : FRealCurve
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FSimpleCurve);

    DEFINE_STRUCT_PROP(Keys, TArray<FSimpleCurveKey>);
};

class UCurveTable : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UCurveTable);

    TMap<FName, FRealCurve*>& GetRowMap() const
    {
        return *(TMap<FName, FRealCurve*>*)(__int64(this) + 0x30);
    }
    __declspec(property(get = GetRowMap))
    TMap<FName, FRealCurve*> RowMap;
};

struct FCurveTableRowHandle
{
    UCurveTable* CurveTable;
    FName RowName;
};