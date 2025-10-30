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

	auto Time = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
	auto WarmupDuration = 90.f;

	GamePhaseLogic->WarmupCountdownStartTime = Time;
	GamePhaseLogic->WarmupCountdownEndTime = Time + WarmupDuration;
	GamePhaseLogic->WarmupCountdownDuration = 10.f;
	GamePhaseLogic->WarmupEarlyCountdownDuration = WarmupDuration - 10.f;
	auto GameState = (AFortGameStateAthena*)GameMode->GameState;

	if (GameState->MapInfo->FlightInfos.Num() > 0)
	{
		TArray<AFortAthenaAircraft*> Aircrafts;
		auto& FlightInfo = GameState->MapInfo->FlightInfos[0];
		Aircrafts.Add(AFortAthenaAircraft::SpawnAircraft(UWorld::GetWorld(), GameState->MapInfo->AircraftClass, FlightInfo));
		GamePhaseLogic->SetAircrafts(Aircrafts);
		GamePhaseLogic->OnRep_Aircrafts();
	}

	GamePhaseLogic->SetGamePhase(EAthenaGamePhase::Warmup);
	GamePhaseLogic->SetGamePhaseStep(EAthenaGamePhaseStep::Warmup);
}

AFortSafeZoneIndicator* UFortGameStateComponent_BattleRoyaleGamePhaseLogic::SetupSafeZoneIndicator()
{
	// thanks heliato

	if (!this->SafeZoneIndicator)
	{
		AFortSafeZoneIndicator* SafeZoneIndicator = UWorld::SpawnActor<AFortSafeZoneIndicator>(SafeZoneIndicatorClass, FVector{});

		if (SafeZoneIndicator)
		{
			auto GameState = (AFortGameStateAthena*) UWorld::GetWorld()->GameState;
			FFortSafeZoneDefinition& SafeZoneDefinition = GameState->MapInfo->SafeZoneDefinition;
			float SafeZoneCount = SafeZoneDefinition.Count.Evaluate();

			auto& Array = SafeZoneIndicator->SafeZonePhases;


			if (Array.IsValid())
				Array.Free();

			const float Time = (float)UGameplayStatics::GetTimeSeconds(GameState);

			for (float i = 0; i < SafeZoneCount; i++)
			{
				auto PhaseInfo = (FFortSafeZonePhaseInfo*)malloc(FFortSafeZonePhaseInfo::Size());
				__stosb((PBYTE)PhaseInfo, 0, FFortSafeZonePhaseInfo::Size());

				PhaseInfo->Radius = SafeZoneDefinition.Radius.Evaluate(i);
				PhaseInfo->WaitTime = SafeZoneDefinition.WaitTime.Evaluate(i);
				PhaseInfo->ShrinkTime = SafeZoneDefinition.ShrinkTime.Evaluate(i);
				PhaseInfo->PlayerCap = (int)SafeZoneDefinition.PlayerCapSolo.Evaluate(i);

				UDataTableFunctionLibrary::EvaluateCurveTableRow(GameState->AthenaGameDataTable, UKismetStringLibrary::Conv_StringToName(FString(L"Default.SafeZone.Damage")), i, nullptr, &PhaseInfo->DamageInfo.Damage, FString());
				if (i == 0.f)
					PhaseInfo->DamageInfo.Damage = 0.01f;
				PhaseInfo->DamageInfo.bPercentageBasedDamage = true;
				PhaseInfo->TimeBetweenStormCapDamage = TimeBetweenStormCapDamage.Evaluate(i);
				PhaseInfo->StormCapDamagePerTick = StormCapDamagePerTick.Evaluate(i);
				PhaseInfo->StormCampingIncrementTimeAfterDelay = StormCampingIncrementTimeAfterDelay.Evaluate(i);
				PhaseInfo->StormCampingInitialDelayTime = StormCampingInitialDelayTime.Evaluate(i);
				PhaseInfo->MegaStormGridCellThickness = (int)SafeZoneDefinition.MegaStormGridCellThickness.Evaluate(i);

				if (FFortSafeZonePhaseInfo::HasUsePOIStormCenter())
					PhaseInfo->UsePOIStormCenter = false;

				PhaseInfo->Center = FVector{};

				Array.Add(*PhaseInfo, FFortSafeZonePhaseInfo::Size());
				free(PhaseInfo);

				SafeZoneIndicator->PhaseCount++;
			}

			SafeZoneIndicator->OnRep_PhaseCount();

			SafeZoneIndicator->SafeZoneStartShrinkTime = Time + Array[0].WaitTime;
			SafeZoneIndicator->SafeZoneFinishShrinkTime = SafeZoneIndicator->SafeZoneStartShrinkTime + Array[0].ShrinkTime;

			SafeZoneIndicator->CurrentPhase = 0;
			SafeZoneIndicator->OnRep_CurrentPhase();
		}

		this->SafeZoneIndicator = SafeZoneIndicator;
		OnRep_SafeZoneIndicator();
	}

	return this->SafeZoneIndicator;
}

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::StartNewSafeZonePhase(int NewSafeZonePhase)
{
	float TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
	auto& Array = SafeZoneIndicator->SafeZonePhases;

	if (Array.IsValidIndex(NewSafeZonePhase))
	{
		if (Array.IsValidIndex(NewSafeZonePhase - 1))
		{
			auto& PreviousPhaseInfo = Array.Get(NewSafeZonePhase - 1, FFortSafeZonePhaseInfo::Size());

			SafeZoneIndicator->PreviousCenter = PreviousPhaseInfo.Center;
			SafeZoneIndicator->PreviousRadius = PreviousPhaseInfo.Radius;
		}

		auto& PhaseInfo = Array.Get(NewSafeZonePhase, FFortSafeZonePhaseInfo::Size());

		SafeZoneIndicator->NextCenter = PhaseInfo.Center;
		SafeZoneIndicator->NextRadius = PhaseInfo.Radius;
		SafeZoneIndicator->NextMegaStormGridCellThickness = PhaseInfo.MegaStormGridCellThickness;

		if (Array.IsValidIndex(NewSafeZonePhase + 1))
		{
			auto& NextPhaseInfo = Array.Get(NewSafeZonePhase + 1, FFortSafeZonePhaseInfo::Size());

			SafeZoneIndicator->FutureReplicator->NextNextCenter = NextPhaseInfo.Center;
			SafeZoneIndicator->FutureReplicator->NextNextRadius = NextPhaseInfo.Radius;

			SafeZoneIndicator->NextNextCenter = NextPhaseInfo.Center;
			SafeZoneIndicator->NextNextRadius = NextPhaseInfo.Radius;
			SafeZoneIndicator->NextNextMegaStormGridCellThickness = NextPhaseInfo.MegaStormGridCellThickness;
		}

		SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds + PhaseInfo.WaitTime;
		SafeZoneIndicator->SafeZoneFinishShrinkTime = SafeZoneIndicator->SafeZoneStartShrinkTime + PhaseInfo.ShrinkTime;

		SafeZoneIndicator->CurrentDamageInfo = PhaseInfo.DamageInfo;
		SafeZoneIndicator->OnRep_CurrentDamageInfo();

		SafeZoneIndicator->CurrentPhase = NewSafeZonePhase;
		SafeZoneIndicator->OnRep_CurrentPhase();

		SafeZoneIndicator->OnSafeZonePhaseChanged.Process();

		SetGamePhaseStep(EAthenaGamePhaseStep::StormHolding);
		SafeZoneIndicator->ForceNetUpdate();
	}
}


uint64_t Reset_ = 0;
void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Tick()
{
	auto Time = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

	static bool gettingReady = false;
	if (!gettingReady)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && WarmupEarlyCountdownDuration < Time)
		{
			gettingReady = true;

			SetGamePhaseStep(EAthenaGamePhaseStep::GetReady);
		}
	}

	static bool startedBus = false;
	if (gettingReady && !startedBus)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && WarmupCountdownEndTime < Time)
		{
			startedBus = true;

			auto Aircraft = Aircrafts_GameState[0].Get();

			auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
			auto& FlightInfo = GameState->MapInfo->FlightInfos[0];

			Aircraft->FlightElapsedTime = 0;
			Aircraft->DropStartTime = (float)Time + FlightInfo.TimeTillDropStart;
			Aircraft->DropEndTime = (float)Time + FlightInfo.TimeTillDropEnd;
			Aircraft->FlightStartTime = (float)Time;
			Aircraft->FlightEndTime = (float)Time + FlightInfo.TimeTillFlightEnd;
			Aircraft->ReplicatedFlightTimestamp = (float)Time;
			bAircraftIsLocked = true;

			for (auto& Player__Uncasted : ((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers)
			{
				auto Player = (AFortPlayerControllerAthena*)Player__Uncasted;
				auto Pawn = (AFortPlayerPawnAthena*)Player->Pawn;

				if (Pawn)
				{
					if (Pawn->Role == 3)
					{
						if (Pawn->bIsInAnyStorm)
						{
							Pawn->bIsInAnyStorm = false;
							Pawn->OnRep_IsInAnyStorm();
						}
					}
					Pawn->bIsInsideSafeZone = true;
					Pawn->OnRep_IsInsideSafeZone();
					Pawn->OnEnteredAircraft.Process();
				}

				Player->ClientActivateSlot(0, 0, 0.f, true, true);
				if (Pawn)
					Pawn->K2_DestroyActor();
				auto Reset = (void (*)(AFortPlayerControllerAthena*)) FindReset();
				Reset(Player);
				Player->ClientGotoState(UKismetStringLibrary::Conv_StringToName(FString(L"Spectating")));
			}

			SetGamePhase(EAthenaGamePhase::Aircraft);
			SetGamePhaseStep(EAthenaGamePhaseStep::BusLocked);
		}
	}

	static bool busUnlocked = false;
	if (startedBus && !busUnlocked)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && Aircrafts_GameState[0]->DropStartTime < Time)
		{
			busUnlocked = true;

			bAircraftIsLocked = false;
			SetGamePhaseStep(EAthenaGamePhaseStep::BusFlying);
		}
	}

	static bool startedForming = false;
	if (startedBus && !startedForming)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && Aircrafts_GameState[0]->DropEndTime != -1 && Aircrafts_GameState[0]->DropEndTime < Time)
		{
			startedForming = true;
			auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
			auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

			for (auto& Player__Uncasted : GameMode->AlivePlayers)
			{
				auto Player = (AFortPlayerControllerAthena*)Player__Uncasted;

				if (Player->IsInAircraft())
				{
					Player->GetAircraftComponent()->KickFromAircraft();
				}
			}

			/*if (bLateGame)
			{
				GameState->GamePhase = EAthenaGamePhase::SafeZones;
				GameState->GamePhaseStep = EAthenaGamePhaseStep::StormHolding;
				GameState->OnRep_GamePhase(EAthenaGamePhase::Aircraft);
			}*/
			SafeZonesStartTime = (float)Time + 60.f;

			SetGamePhase(EAthenaGamePhase::SafeZones);
			SetGamePhaseStep(EAthenaGamePhaseStep::StormForming);
		}
	}

	static bool finishedFlight = false;
	if (busUnlocked && !finishedFlight)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && Aircrafts_GameState[0]->FlightEndTime != -1 && Aircrafts_GameState[0]->FlightEndTime < Time)
		{
			finishedFlight = true;
			auto Aircraft = Aircrafts_GameState[0].Get();

			Aircraft->K2_DestroyActor();
			Aircrafts_GameState.Clear();
			Aircrafts_GameMode.Clear();
		}
	}

	static bool formedZone = false;
	if (finishedFlight && !formedZone)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && SafeZonesStartTime != -1 && SafeZonesStartTime < Time)
		{
			formedZone = true;
			auto SafeZoneIndicator = SetupSafeZoneIndicator();
			StartNewSafeZonePhase(1);
		}
	}

	static bool bUpdatedPhase = false;
	if (formedZone)
	{
		if (!bUpdatedPhase && SafeZoneIndicator->SafeZoneStartShrinkTime < Time)
		{
			bUpdatedPhase = true;
			SetGamePhaseStep(EAthenaGamePhaseStep::StormShrinking);
		}
		else if (SafeZoneIndicator->SafeZoneFinishShrinkTime < Time)
		{
			StartNewSafeZonePhase(SafeZoneIndicator->CurrentPhase + 1);
			bUpdatedPhase = false;
		}
	}
}

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Hook()
{
	if (!GetDefaultObj())
		return;
	Reset_ = FindReset();

	Utils::Hook(FindHandleMatchHasStarted(), HandleMatchHasStarted, HandleMatchHasStartedOG);
}