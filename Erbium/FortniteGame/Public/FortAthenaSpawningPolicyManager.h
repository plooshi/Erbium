#pragma once
#include "../../pch.h"

class AFortAthenaSpawningPolicyManager : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaSpawningPolicyManager);

    DEFINE_PROP(GameModeAthena, AActor*);
    DEFINE_PROP(GameStateAthena, AActor*);
};