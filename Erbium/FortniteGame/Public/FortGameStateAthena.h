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

    DEFINE_FUNC(OnRep_CurrentPlaylistInfo, void);
    DEFINE_FUNC(OnRep_CurrentPlaylistData, void);
    DEFINE_FUNC(OnRep_CurrentPlaylistId, void);

    DEFINE_FUNC(OnRep_AdditionalPlaylistLevelsStreamed, void);
};