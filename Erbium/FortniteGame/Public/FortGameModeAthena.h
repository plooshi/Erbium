#pragma once
#include "../../pch.h"
#include "FortGameStateAthena.h"
#include "FortGameSessionAthena.h"
#include "FortSafeZoneIndicator.h"
#include "FortInventory.h"
#include "FortPlayerPawnAthena.h"
#include "FortPlayerControllerAthena.h"

class AFortGameModeAthena : public AActor
{
public:
    static inline uint8_t CurrentTeam = 3;
    static inline uint8_t PlayersOnCurTeam = 0;
    static inline TArray<const UFortAbilitySet*> AbilitySets;

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
    DEFINE_PROP(AthenaGameDataTable, UCurveTable*);
    DEFINE_PROP(RedirectAthenaLootTierGroups, TMap<FName, FName>);
    DEFINE_PROP(WarmupCountdownDuration, float);
    DEFINE_PROP(WarmupEarlyCountdownDuration, float);
    DEFINE_PROP(SafeZoneLocations, TArray<FVector>);
    DEFINE_PROP(DefaultPawnClass, const UClass*);
    DEFINE_PROP(PlayerControllerClass, const UClass*);

    DEFINE_FUNC(SpawnDefaultPawnAtTransform, AFortPlayerPawnAthena*);
    DEFINE_FUNC(RestartPlayer, void);
    DEFINE_FUNC(ReadyToStartMatch, bool);
    DEFINE_FUNC(HandleStartingNewPlayer, void);
    DEFINE_FUNC(OnAircraftExitedDropZone, void);

    DefUHookOgRet(bool, ReadyToStartMatch_);
    static void SpawnDefaultPawnFor(UObject*, FFrame&, AActor**);
    DefHookOg(void, HandlePostSafeZonePhaseChanged, AFortGameModeAthena*, int);
    DefHookOg(uint8_t, PickTeam, AFortGameModeAthena*, uint8_t, AFortPlayerControllerAthena*);
    DefUHookOg(HandleStartingNewPlayer_);
    DefHookOg(bool, StartAircraftPhase, AFortGameModeAthena*, char);
    DefUHookOg(OnAircraftExitedDropZone_);
    
    InitHooks;
};