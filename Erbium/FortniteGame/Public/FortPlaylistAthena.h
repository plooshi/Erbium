#pragma once
#include "../../pch.h"
#include "../../Engine/Public/DataTable.h"


class UFortPlaylistAthena : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortPlaylistAthena);

    DEFINE_PROP(PlaylistName, FName);
    DEFINE_PROP(PlaylistId, int32);
    DEFINE_PROP(MaxPlayers, int32);
    DEFINE_PROP(AdditionalLevels, TArray<TSoftObjectPtr<UWorld>>);
    DEFINE_PROP(AdditionalLevelsServerOnly, TArray<TSoftObjectPtr<UWorld>>);
    DEFINE_PROP(GarbageCollectionFrequency, float);
    DEFINE_PROP(MaxSquadSize, int32);
    DEFINE_PROP(LootTierData, TSoftObjectPtr<UDataTable>);
    DEFINE_PROP(LootPackages, TSoftObjectPtr<UDataTable>);
};