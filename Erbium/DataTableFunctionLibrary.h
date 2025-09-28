#pragma once
#include "pch.h"
#include "Utils.h"

class UDataTableFunctionLibrary : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UDataTableFunctionLibrary);

    DEFINE_STATIC_FUNC(EvaluateCurveTableRow, void);
};