#pragma once
#include "../../pch.h"
#include "FortGameMode.h"

enum class EAthenaGamePhase : uint8
{
	None = 0,
	Setup = 1,
	Warmup = 2,
	Aircraft = 3,
	SafeZones = 4,
	EndGame = 5,
	Count = 6,
};

enum class EAthenaGamePhaseStep : uint8
{
	None = 0,
	Setup = 1,
	Warmup = 2,
	GetReady = 3,
	BusLocked = 4,
	BusFlying = 5,
	StormForming = 6,
	StormHolding = 7,
	StormShrinking = 8,
	Countdown = 9,
	FinalCountdown = 10,
	EndGame = 11,
	Count = 12,
};


class UFortGameStateComponent_BattleRoyaleGamePhaseLogic : public UActorComponent
{
public:
    UCLASS_COMMON_MEMBERS(UFortGameStateComponent_BattleRoyaleGamePhaseLogic);
	static inline bool bSkipWarmup = false;
	static inline bool bSkipAircraft = false;
	static inline bool bEnableZones = true;
	static inline bool bPausedZone = false;
	static inline bool bStartAircraft = false;

	DEFINE_PROP(WarmupCountdownStartTime, float);
	DEFINE_PROP(WarmupCountdownEndTime, float);
	DEFINE_PROP(WarmupCountdownDuration, float);
	DEFINE_PROP(WarmupEarlyCountdownDuration, float);
	DEFINE_PROP(Aircrafts_GameState, TArray<TWeakObjectPtr<AFortAthenaAircraft>>);
	DEFINE_PROP(Aircrafts_GameMode, TArray<TWeakObjectPtr<AFortAthenaAircraft>>);
	DEFINE_PROP(bAircraftIsLocked, bool);
	DEFINE_PROP(SafeZonesStartTime, float);
	DEFINE_PROP(SafeZoneIndicator, AFortSafeZoneIndicator*);
	DEFINE_PROP(SafeZoneIndicatorClass, TSubclassOf<AFortSafeZoneIndicator>);
	DEFINE_PROP(TimeBetweenStormCapDamage, FScalableFloat);
	DEFINE_PROP(StormCapDamagePerTick, FScalableFloat);
	DEFINE_PROP(StormCampingIncrementTimeAfterDelay, FScalableFloat);
	DEFINE_PROP(StormCampingInitialDelayTime, FScalableFloat);
	DEFINE_PROP(bSafeZoneActive, bool);
	DEFINE_PROP(bSafeZonePaused, bool);

	DEFINE_FUNC(OnRep_GamePhase, void);
	DEFINE_FUNC(HandleGamePhaseStepChanged, void);
	DEFINE_FUNC(SetAircrafts, void);
	DEFINE_FUNC(OnRep_Aircrafts, void);
	DEFINE_FUNC(OnRep_SafeZoneIndicator, void);
	DEFINE_FUNC(IsInCurrentSafeZone, bool);

    DEFINE_STATIC_FUNC(Get, UFortGameStateComponent_BattleRoyaleGamePhaseLogic*);

	void SetGamePhase(EAthenaGamePhase GamePhase);
	void SetGamePhaseStep(EAthenaGamePhaseStep GamePhaseStep);
	DefHookOg(void, HandleMatchHasStarted, AFortGameMode*);
	void Tick();
	AFortSafeZoneIndicator* SetupSafeZoneIndicator();
	void StartNewSafeZonePhase(int NewSafeZonePhase, bool bInitial = false);
	static void GenerateStormCircles(AFortAthenaMapInfo* MapInfo);
	void StartAircraftPhase();

    InitHooks;
};