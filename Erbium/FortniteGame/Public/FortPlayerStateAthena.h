#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"
#include "../../Engine/Public/AbilitySystemComponent.h"

struct FUniqueNetIdRepl
{
public:
    uint8 _Padding[0x18];
    TArray<uint8> ReplicationBytes;
};

class AFortPlayerStateAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPlayerStateAthena);

    DEFINE_PROP(AbilitySystemComponent, UAbilitySystemComponent*);
    DEFINE_PROP(SquadId, int32);
    DEFINE_PROP(TeamIndex, int32);
    DEFINE_PROP(UniqueId, FUniqueNetIdRepl);

    DEFINE_FUNC(OnRep_SquadId, void);
};