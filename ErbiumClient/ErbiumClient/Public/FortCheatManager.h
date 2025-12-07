#pragma once
#include "../../pch.h"

class UFortCheatManager : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortCheatManager);
    
    DEFINE_FUNC(Teleport, void);
};