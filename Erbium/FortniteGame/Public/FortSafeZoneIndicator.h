#pragma once
#include "../../pch.h"

class AFortSafeZoneIndicator : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(AFortSafeZoneIndicator);

    DEFINE_PROP(SafeZoneStartShrinkTime, float);
    DEFINE_PROP(SafeZoneFinishShrinkTime, float);

    DEFINE_FUNC(GetSafeZoneCenter, FVector);
};