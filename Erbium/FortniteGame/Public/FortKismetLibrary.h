#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"
#include "FortInventory.h"

class UFortKismetLibrary : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortKismetLibrary);

    DEFINE_STATIC_FUNC(UpdatePlayerCustomCharacterPartsVisualization, void);
    DEFINE_STATIC_FUNC(K2_GetResourceItemDefinition, UFortItemDefinition*);
};