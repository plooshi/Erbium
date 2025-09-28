#pragma once
#include "pch.h"
#include "Utils.h"
#include "FortGameStateAthena.h"
#include "FortGameSessionAthena.h"
#include "FortSafeZoneIndicator.h"
#include "FortInventory.h"
#include "FortPlayerPawnAthena.h"

class AFortGameModeAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortGameModeAthena);

    DEFINE_PROP(CurrentPlaylistId, int32);
    DEFINE_PROP(WarmupRequiredPlayerCount, int32);
    DEFINE_BITFIELD_PROP(bWorldIsReady);
    DEFINE_PROP(GameSession, AFortGameSession*);
    DEFINE_PROP(CurrentPlaylistName, FName);
    DEFINE_PROP(GameState, AFortGameStateAthena*);
    DEFINE_PROP(AlivePlayers, TArray<AActor*>);
    DEFINE_PROP(SafeZoneIndicator, AFortSafeZoneIndicator*);
    DEFINE_PROP(SafeZonePhase, int32);
    DEFINE_PROP(StartingItems, TArray<FItemAndCount>);
    DEFINE_PROP(bDisableGCOnServerDuringMatch, bool);
    DEFINE_PROP(bPlaylistHotfixChangedGCDisabling, bool);

    DEFINE_FUNC(SpawnDefaultPawnAtTransform, AFortPlayerPawnAthena*);
    DEFINE_FUNC(RestartPlayer, void);
    DEFINE_FUNC(ReadyToStartMatch, bool);

    DefUHookOgRet(bool, ReadyToStartMatch_);
    static void SpawnDefaultPawnFor(UObject*, FFrame&, AActor**);
    DefHookOg(void, HandlePostSafeZonePhaseChanged, AFortGameModeAthena*, int);
    
    InitHooks;
};