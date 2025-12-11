#pragma once
#include "../../pch.h"
#include "DataTableFunctionLibrary.h"

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
    const UCurveTable* CurveTable;
    FName RowName;
};

struct FScalableFloat
{
public:
    float Value;
    uint8 _Padding[0x4];
    FCurveTableRowHandle Curve;
    uint8 _Padding2[0x10];

    inline float Evaluate(float i = 0.f)
    {
        if (!Curve.CurveTable || !Curve.RowName.IsValid())
            return Value;

        float Out = 0.f;
        UDataTableFunctionLibrary::EvaluateCurveTableRow(Curve.CurveTable, Curve.RowName, i, nullptr, &Out, FString());
        return Value * Out;
    }
};