#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"

class UFortMissionLibrary : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortMissionLibrary);

    static void TeleportPlayerPawn(UObject* Context, FFrame& Stack, bool* Ret);

    InitHooks;
};