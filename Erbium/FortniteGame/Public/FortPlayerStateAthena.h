#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"
#include "../../Engine/Public/AbilitySystemComponent.h"

class AFortPlayerStateAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPlayerStateAthena);

    DEFINE_PROP(AbilitySystemComponent, UAbilitySystemComponent*);
};