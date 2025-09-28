#pragma once
#include "pch.h"
#include "Utils.h"

class UFortKismetLibrary : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortKismetLibrary);

    DEFINE_STATIC_FUNC(UpdatePlayerCustomCharacterPartsVisualization, void);
};