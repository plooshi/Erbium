#include "pch.h"
#include "../Public/BattleRoyaleGamePhaseLogic.h"


void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::SetGamePhase(EAthenaGamePhase GamePhase)
{
	static auto GamePhaseOffset = this->GetOffset("GamePhase");
	auto& _GamePhase = *(EAthenaGamePhase*)(__int64(this) + GamePhaseOffset);

	auto OldGamePhase = _GamePhase;
	_GamePhase = GamePhase;
	OnRep_GamePhase(OldGamePhase);
}

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::SetGamePhaseStep(EAthenaGamePhaseStep GamePhaseStep)
{
	static auto GamePhaseStepOffset = this->GetOffset("GamePhaseStep");
	auto& _GamePhaseStep = *(EAthenaGamePhaseStep*)(__int64(this) + GamePhaseStepOffset);

	_GamePhaseStep = GamePhaseStep;
	HandleGamePhaseStepChanged(GamePhaseStep);
}

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::HandleMatchHasStarted(AFortGameModeAthena* GameMode)
{
	HandleMatchHasStartedOG(GameMode);
	auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameMode);

	GamePhaseLogic->SetGamePhase(EAthenaGamePhase::Warmup);
	GamePhaseLogic->SetGamePhaseStep(EAthenaGamePhaseStep::Warmup);
}

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Hook()
{
	Utils::Hook(FindHandleMatchHasStarted(), HandleMatchHasStarted, HandleMatchHasStartedOG);
}