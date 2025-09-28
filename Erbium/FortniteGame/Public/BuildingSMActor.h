#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"

class ABuildingSMActor : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ABuildingSMActor);
    
    //DefHookOg(void, OnDamageServer, ABuildingSMActor*, float, FGameplayTagContainer, FVector, __int64, AFortPlayerControllerAthena*, AActor*, FGameplayEffectContextHandle);

    InitHooks;
};