#pragma once
#include "../../pch.h"
#include "../../Engine/Public/AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

struct FUniqueNetIdRepl
{
public:
    uint8 _Padding[0x18];
    TArray<uint8> ReplicationBytes;
};

struct FDeathInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FDeathInfo);

    DEFINE_STRUCT_PROP(Killer, AActor*);
    DEFINE_STRUCT_PROP(bDBNO, bool);
    DEFINE_STRUCT_PROP(DeathLocation, FVector);
    DEFINE_STRUCT_PROP(DeathTags, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(DeathCause, uint8);
    DEFINE_STRUCT_PROP(Downer, AActor*);
    DEFINE_STRUCT_PROP(FinisherOrDowner, AActor*);
    DEFINE_STRUCT_PROP(Distance, float);
    DEFINE_STRUCT_PROP(bInitialized, bool);
    DEFINE_STRUCT_PROP(FinisherOrDownerTags, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(VictimTags, FGameplayTagContainer);
};

class AFortPlayerStateAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPlayerStateAthena);

    DEFINE_PROP(AbilitySystemComponent, UAbilitySystemComponent*);
    DEFINE_PROP(SquadId, int32);
    DEFINE_PROP(TeamIndex, int32);
    DEFINE_PROP(UniqueId, FUniqueNetIdRepl);
    DEFINE_PROP(PawnDeathLocation, FVector);
    DEFINE_PROP(DeathInfo, FDeathInfo);
    DEFINE_PROP(Kills, int32);
    DEFINE_PROP(KillScore, int32);
    DEFINE_PROP(TeamKillScore, int32);
    DEFINE_PROP(Place, int32);

    DEFINE_FUNC(OnRep_SquadId, void);
    DEFINE_FUNC(OnRep_DeathInfo, void);
    DEFINE_STATIC_FUNC(ToDeathCause, uint8);
    DEFINE_FUNC(OnRep_Kills, void);
    DEFINE_FUNC(OnRep_TeamKillScore, void);
    DEFINE_FUNC(ClientReportKill, void);
    DEFINE_FUNC(ClientReportTeamKill, void);
    DEFINE_FUNC(OnRep_Place, void);
};