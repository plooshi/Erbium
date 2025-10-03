#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"
#include "FortPlaylistAthena.h"
#include "../../Engine/Public/CurveTable.h"
#include "FortPlayerStateAthena.h"

struct FGameMemberInfo : public FFastArraySerializerItem
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameMemberInfo);

    DEFINE_STRUCT_PROP(TeamIndex, int32);
    DEFINE_STRUCT_PROP(SquadId, int32);
    DEFINE_STRUCT_PROP(MemberUniqueId, FUniqueNetIdRepl);
};

struct FGameMemberInfoArray : public FFastArraySerializer
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameMemberInfoArray);

    DEFINE_STRUCT_PROP(Members, TArray<FGameMemberInfo>);
};

struct FAdditionalLevelStreamed
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FAdditionalLevelStreamed);

    DEFINE_STRUCT_PROP(LevelName, FName);
    DEFINE_STRUCT_PROP(bIsServerOnly, bool);
};

struct FPlaylistPropertyArray : public FFastArraySerializer
{
    USCRIPTSTRUCT_COMMON_MEMBERS(FPlaylistPropertyArray);

    DEFINE_STRUCT_PROP(PlaylistReplicationKey, int32);
    DEFINE_STRUCT_PROP(BasePlaylist, const UFortPlaylistAthena*);
    DEFINE_STRUCT_PROP(OverridePlaylist, const UFortPlaylistAthena*);
};

class AFortAthenaMapInfo : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaMapInfo);

    DEFINE_PROP(LlamaClass, UClass*);
};

struct FAircraftFlightInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FAircraftFlightInfo);

    DEFINE_STRUCT_PROP(FlightSpeed, float);
    DEFINE_STRUCT_PROP(FlightStartLocation, FVector);
    DEFINE_STRUCT_PROP(TimeTillFlightEnd, float);
    DEFINE_STRUCT_PROP(TimeTillDropStart, float);
    DEFINE_STRUCT_PROP(TimeTillDropEnd, float);
};

class AFortAthenaAircraft : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaAircraft);

    DEFINE_PROP(FlightInfo, FAircraftFlightInfo);
    DEFINE_PROP(FlightStartTime, float);
    DEFINE_PROP(FlightEndTime, float);
    DEFINE_PROP(FlightSpeed, float);
    DEFINE_PROP(FlightStartLocation, FVector);
    DEFINE_PROP(TimeTillFlightEnd, float);
    DEFINE_PROP(TimeTillDropStart, float);
    DEFINE_PROP(TimeTillDropEnd, float);
};

class AFortGameStateAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortGameStateAthena);

    DEFINE_PROP(CurrentPlaylistInfo, FPlaylistPropertyArray);
    DEFINE_PROP(CurrentPlaylistId, int32);
    DEFINE_PROP(MapInfo, AFortAthenaMapInfo*);
    DEFINE_PROP(AdditionalPlaylistLevelsStreamed, TArray<FAdditionalLevelStreamed>);
    DEFINE_PROP(DefaultParachuteDeployTraceForGroundDistance, float);
    DEFINE_PROP(AthenaGameDataTable, UCurveTable*);
    DEFINE_PROP(CurrentPlaylistData, const UFortPlaylistAthena*);
    DEFINE_PROP(GameMemberInfoArray, FGameMemberInfoArray);
    DEFINE_PROP(AllPlayerBuildableClassesIndexLookup, TMap<TSubclassOf<AActor>, int32>);
    DEFINE_PROP(AllPlayerBuildableClasses, TArray<TSubclassOf<AActor>>);
    DEFINE_PROP(WorldLevel, int32);
    DEFINE_PROP(WarmupCountdownStartTime, float);
    DEFINE_PROP(WarmupCountdownEndTime, float);
    DEFINE_PROP(PlayersLeft, int32);
    DEFINE_PROP(WinningTeam, int32);
    DEFINE_PROP(WinningPlayerState, AFortPlayerStateAthena*);
    DEFINE_PROP(GamePhase, uint8);
    DEFINE_PROP(GamePhaseStep, uint8);
    DEFINE_PROP(Aircrafts, TArray<AFortAthenaAircraft*>);
    DEFINE_PROP(Aircraft, AFortAthenaAircraft*);
    DEFINE_PROP(SafeZonesStartTime, float);

    DEFINE_FUNC(OnRep_CurrentPlaylistInfo, void);
    DEFINE_FUNC(OnRep_CurrentPlaylistData, void);
    DEFINE_FUNC(OnRep_CurrentPlaylistId, void);
    DEFINE_FUNC(OnRep_AdditionalPlaylistLevelsStreamed, void);
    DEFINE_FUNC(IsRespawningAllowed, bool);
    DEFINE_FUNC(OnRep_WinningTeam, void);
    DEFINE_FUNC(OnRep_WinningPlayerState, void);
    DEFINE_FUNC(OnRep_GamePhase, void);
};