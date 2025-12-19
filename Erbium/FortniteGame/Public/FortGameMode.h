#pragma once
#include "../../pch.h"
#include "FortGameStateAthena.h"
#include "FortGameSessionAthena.h"
#include "FortSafeZoneIndicator.h"
#include "FortInventory.h"
#include "FortPlayerPawnAthena.h"
#include "FortPlayerControllerAthena.h"

class AFortGameMode : public AActor
{
public:
    static inline uint8_t CurrentTeam = 3;
    static inline uint8_t PlayersOnCurTeam = 0;
    static inline TArray<const UFortAbilitySet*> AbilitySets;
    static inline FVector SafeZoneLoc{};

    UCLASS_COMMON_MEMBERS(AFortGameMode);

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
    DEFINE_PROP(PlaylistHotfixOriginalGCFrequency, float);
    DEFINE_PROP(SafeZoneIndicatorClass, TSubclassOf<AFortSafeZoneIndicator>);
    DEFINE_PROP(TimeBetweenStormCapDamage, FScalableFloat);
    DEFINE_PROP(StormCapDamagePerTick, FScalableFloat);
    DEFINE_PROP(StormCampingIncrementTimeAfterDelay, FScalableFloat);
    DEFINE_PROP(StormCampingInitialDelayTime, FScalableFloat);
    DEFINE_PROP(bSafeZoneActive, bool);
    DEFINE_PROP(bSafeZonePaused, bool);
    DEFINE_PROP(OnSafeZoneIndicatorSpawned, TMulticastInlineDelegate<void(AFortSafeZoneIndicator*)>);
    DEFINE_PROP(MatchState, FName);
    DEFINE_PROP(bEnableDBNO, bool);
    DEFINE_PROP(AIDirector, AActor*);
    DEFINE_PROP(AIGoalManager, AActor*);
    DEFINE_PROP(bEnableReplicationGraph, bool);
    DEFINE_PROP(bAllowSpectateAfterDeath, bool);
    DEFINE_PROP(ServerBotManager, UObject*);

    DEFINE_FUNC(SpawnDefaultPawnAtTransform, AFortPlayerPawnAthena*);
    DEFINE_FUNC(RestartPlayer, void);
    DEFINE_FUNC(ReadyToStartMatch, bool);
    DEFINE_FUNC(HandleStartingNewPlayer, void);
    DEFINE_FUNC(OnAircraftExitedDropZone, void);
    DEFINE_FUNC(ChangeName, void);
    DEFINE_FUNC(ChoosePlayerStart, AActor*);
    DEFINE_FUNC(GetDefaultPawnClassForController, const UClass*);
    DEFINE_FUNC(IsInCurrentSafeZone, bool);

    DefUHookOgRet(bool, ReadyToStartMatch_);
    static void SpawnDefaultPawnFor(UObject*, FFrame&, AActor**);
    DefHookOg(void, HandlePostSafeZonePhaseChanged, AFortGameMode*, int);
    DefHookOg(uint8_t, PickTeam, AFortGameMode*, uint8_t, AFortPlayerControllerAthena*);
    DefUHookOg(HandleStartingNewPlayer_);
    DefHookOg(bool, StartAircraftPhase, AFortGameMode*, char);
    DefUHookOg(OnAircraftExitedDropZone_);
    
    InitHooks;
    InitPostLoadHooks;
};

class AFortGameModeAthena : public AFortGameMode
{
public:
    UCLASS_COMMON_MEMBERS(AFortGameModeAthena);
};