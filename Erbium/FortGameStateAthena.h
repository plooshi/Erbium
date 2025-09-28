#pragma once
#include "pch.h"
#include "Utils.h"
#include "FortPlaylistAthena.h"
#include "CurveTable.h"

struct FAdditionalLevelStreamed final
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

class AFortGameStateAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortGameStateAthena);

    DEFINE_PROP(CurrentPlaylistInfo, FPlaylistPropertyArray);
    DEFINE_PROP(CurrentPlaylistId, int32);
    DEFINE_PROP(MapInfo, AActor*);
    DEFINE_PROP(AdditionalPlaylistLevelsStreamed, TArray<FAdditionalLevelStreamed>);
    DEFINE_PROP(DefaultParachuteDeployTraceForGroundDistance, float);
    DEFINE_PROP(AthenaGameDataTable, UCurveTable*);
    DEFINE_PROP(CurrentPlaylistData, const UFortPlaylistAthena*);

    DEFINE_FUNC(OnRep_CurrentPlaylistInfo, void);
    DEFINE_FUNC(OnRep_CurrentPlaylistData, void);
    DEFINE_FUNC(OnRep_CurrentPlaylistId, void);

    DEFINE_FUNC(OnRep_AdditionalPlaylistLevelsStreamed, void);
};