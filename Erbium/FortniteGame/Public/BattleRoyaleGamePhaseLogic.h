#pragma once
#include "../../pch.h"
#include "FortGameModeAthena.h"

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

	DEFINE_FUNC(OnRep_GamePhase, void);
	DEFINE_FUNC(HandleGamePhaseStepChanged, void);

    DEFINE_STATIC_FUNC(Get, UFortGameStateComponent_BattleRoyaleGamePhaseLogic*);

	void SetGamePhase(EAthenaGamePhase GamePhase);
	void SetGamePhaseStep(EAthenaGamePhaseStep GamePhaseStep);
	DefHookOg(void, HandleMatchHasStarted, AFortGameModeAthena*);

    InitHooks;
};