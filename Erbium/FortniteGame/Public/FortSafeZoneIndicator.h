#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"


class AFortSafeZoneIndicator : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(AFortSafeZoneIndicator);

    DEFINE_PROP(SafeZoneStartShrinkTime, float);
    DEFINE_PROP(SafeZoneFinishShrinkTime, float);
};