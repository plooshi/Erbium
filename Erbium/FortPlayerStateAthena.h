#pragma once
#include "pch.h"
#include "Utils.h"
#include "AbilitySystemComponent.h"

class AFortPlayerStateAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPlayerStateAthena);

    DEFINE_PROP(AbilitySystemComponent, UAbilitySystemComponent*);
};