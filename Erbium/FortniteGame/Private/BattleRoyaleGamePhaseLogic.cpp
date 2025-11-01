#include "pch.h"
#include "../Public/BattleRoyaleGamePhaseLogic.h"

uint64_t SetGamePhase_ = 0;
void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::SetGamePhase(EAthenaGamePhase GamePhase)
{
	auto SetGamePhaseInternal = (void(*)(UFortGameStateComponent_BattleRoyaleGamePhaseLogic*, EAthenaGamePhase)) SetGamePhase_;

	if (SetGamePhaseInternal)
		return SetGamePhaseInternal(this, GamePhase);
	else
	{
		static auto GamePhaseOffset = this->GetOffset("GamePhase");
		auto& _GamePhase = *(EAthenaGamePhase*)(__int64(this) + GamePhaseOffset);

		auto OldGamePhase = _GamePhase;
		_GamePhase = GamePhase;
		OnRep_GamePhase(OldGamePhase);
	}
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

	GamePhaseLogic->SetGamePhase(EAthenaGamePhase::Warmup);
	GamePhaseLogic->SetGamePhaseStep(EAthenaGamePhaseStep::Warmup);
}

constexpr float KINDA_SMALL_NUMBER = 1.e-4f;

// thanks mariki
struct FStormCircle
{
	FVector Center;
	float Radius;
};

std::vector<FStormCircle> StormCircles;

struct FVector2D
{
public:
	double X;
	double Y;
};

inline FVector2D GetSafeNormal(FVector2D v)
{
	double sizeSq = v.X * v.X + v.Y * v.Y;
	if (sizeSq > KINDA_SMALL_NUMBER)
	{
		double inv = 1. / sqrt(sizeSq);
		return FVector2D(v.X * inv, v.Y * inv);
	}
	return FVector2D(0.f, 0.f);
}

inline FVector GetSafeNormal(FVector v)
{
	double sizeSq = v.X * v.X + v.Y * v.Y + v.Z * v.Z;
	if (sizeSq > KINDA_SMALL_NUMBER)
	{
		double inv = 1. / sqrt(sizeSq);
		return FVector(v.X * inv, v.Y * inv, v.Z * inv);
	}
	return FVector(0.f, 0.f, 0.f);
}

inline bool IsNearlyZero(FVector2D v)
{
	return (v.X * v.X + v.Y * v.Y) < KINDA_SMALL_NUMBER * KINDA_SMALL_NUMBER;
}

FVector ClampToPlayableBounds(const FVector& Candidate, float Radius, const FBoxSphereBounds& Bounds)
{
	FVector Clamped = Candidate;

	Clamped.X = std::clamp(Clamped.X, Bounds.Origin.X - Bounds.BoxExtent.X + Radius, Bounds.Origin.X + Bounds.BoxExtent.X - Radius);
	Clamped.Y = std::clamp(Clamped.Y, Bounds.Origin.Y - Bounds.BoxExtent.Y + Radius, Bounds.Origin.Y + Bounds.BoxExtent.Y - Radius);

	return Clamped;
}

#define INV_PI			(0.31830988618f)
#define HALF_PI			(1.57079632679f)
#define PI 					(3.1415926535897932f)

float RadiansToDegrees(float Radians)
{
	return Radians * (180.0f / PI);
}
inline void SinCos(float* ScalarSin, float* ScalarCos, float  Value)
{
	// Map Value to y in [-pi,pi], x = 2*pi*quotient + remainder.
	float quotient = (INV_PI * 0.5f) * Value;
	if (Value >= 0.0f)
	{
		quotient = (float)((int)(quotient + 0.5f));
	}
	else
	{
		quotient = (float)((int)(quotient - 0.5f));
	}
	float y = Value - (2.0f * PI) * quotient;

	// Map y to [-pi/2,pi/2] with sin(y) = sin(Value).
	float sign;
	if (y > HALF_PI)
	{
		y = PI - y;
		sign = -1.0f;
	}
	else if (y < -HALF_PI)
	{
		y = -PI - y;
		sign = -1.0f;
	}
	else
	{
		sign = +1.0f;
	}

	float y2 = y * y;

	// 11-degree minimax approximation
	*ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

	// 10-degree minimax approximation
	float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
	*ScalarCos = sign * p;
}

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::GenerateStormCircles(AFortAthenaMapInfo* MapInfo)
{
	StormCircles.clear();

	auto FRandSeeded = [&]() { return (float) rand() / 32767.f; };

	float Radii[13] = { 150000, 120000, 95000, 70000, 55000, 32500, 20000, 10000, 5000, 2500, 1650, 1090, 0 };

	FVector FirstCenter = MapInfo->GetMapCenter();
	StormCircles.push_back(FStormCircle{ FirstCenter, Radii[0] });

	float DirAngle = FRandSeeded() * 2.f * PI;
	float DirSin, DirCos;
	SinCos(&DirSin, &DirCos, DirAngle);
	FVector2D Dir(DirCos, DirSin);
	/*for (int i = 1; i < 5; ++i)
	{
		FVector PrevCenter = StormCircles[i - 1].Center;
		float rPrev = StormCircles[i - 1].Radius;
		float rNew = Radii[i];

		double baseAngle = atan2(Dir.Y, Dir.X);
		float delta = (FRandSeeded() - 0.5f) * (PI / 36.f);
		double angle = baseAngle + delta;

		FVector2D MoveDir(cos(angle), sin(angle));
		MoveDir = GetSafeNormal(MoveDir);

		float f_i = 0.4f + FRandSeeded() * 0.5f;
		float Offset = f_i * (rPrev - rNew);

		FVector NewCenter = PrevCenter + FVector(MoveDir.X, MoveDir.Y, 0.f) * Offset;

		StormCircles.push_back({ NewCenter, rNew });
		Dir = MoveDir;
	}*/

	/*for (int i = 5; i < 8; ++i)
	{
		FVector PrevCenter = StormCircles[i - 1].Center;
		float rPrev = StormCircles[i - 1].Radius;
		float rNew = Radii[i];

		float angle = FRandSeeded() * 2.f * PI;
		float s, c; SinCos(&s, &c, angle);
		FVector2D RandDir(c, s);
		RandDir = GetSafeNormal(RandDir);

		float d = sqrt(rPrev * rPrev - rNew * rNew);;
		FVector NewCenter = PrevCenter + FVector(RandDir.X, RandDir.Y, 0.f) * d;

		NewCenter = ClampToPlayableBounds(NewCenter, rNew, MapInfo->CachedPlayableBoundsForClients);

		StormCircles.push_back(FStormCircle{ NewCenter, rNew });
	}*/

	//FVector RefCenter = StormCircles[6].Center;
	//float RefRadius = StormCircles[6].Radius;

	//for (int i = 8; i < 13; ++i)
	for (int i = 1; i < 7; ++i)
	{
		FVector RefCenter = StormCircles[i - 1].Center;
		float RefRadius = StormCircles[i - 1].Radius;
		float angle = FRandSeeded() * 2.f * PI;
		float s, c; SinCos(&s, &c, angle);
		float Dist = FRandSeeded() * RefRadius * 0.4f;
		FVector2D RandDir(c, s);
		RandDir = GetSafeNormal(RandDir);

		FVector NewCenter = RefCenter + FVector(RandDir.X, RandDir.Y, 0.f) * Dist;

		NewCenter = ClampToPlayableBounds(NewCenter, Radii[i], MapInfo->CachedPlayableBoundsForClients);

		StormCircles.push_back(FStormCircle{ NewCenter, Radii[i] });
	}

	for (int i = 7; i < 13; ++i)
	{
		FVector PrevCenter = StormCircles[i - 1].Center;
		float rPrev = StormCircles[i - 1].Radius;
		float rNew = Radii[i];

		float angle = FRandSeeded() * 2.f * PI;
		float s, c; SinCos(&s, &c, angle);
		FVector2D RandDir(c, s);
		RandDir = GetSafeNormal(RandDir);

		float d = sqrt(rPrev * rPrev - rNew * rNew);;
		FVector NewCenter = PrevCenter + FVector(RandDir.X, RandDir.Y, 0.f) * d;

		NewCenter = ClampToPlayableBounds(NewCenter, rNew, MapInfo->CachedPlayableBoundsForClients);

		StormCircles.push_back(FStormCircle{ NewCenter, rNew });
	}
}
// end

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

				PhaseInfo->Center = StormCircles[(int)i].Center;

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

			if (SafeZoneIndicator->FutureReplicator)
			{
				SafeZoneIndicator->FutureReplicator->NextNextCenter = NextPhaseInfo.Center;
				SafeZoneIndicator->FutureReplicator->NextNextRadius = NextPhaseInfo.Radius;
			}

			SafeZoneIndicator->NextNextCenter = NextPhaseInfo.Center;
			SafeZoneIndicator->NextNextRadius = NextPhaseInfo.Radius;
			SafeZoneIndicator->NextNextMegaStormGridCellThickness = NextPhaseInfo.MegaStormGridCellThickness;
		}

		SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds + PhaseInfo.WaitTime;
		SafeZoneIndicator->SafeZoneFinishShrinkTime = SafeZoneIndicator->SafeZoneStartShrinkTime + PhaseInfo.ShrinkTime;

		SafeZoneIndicator->CurrentDamageInfo = PhaseInfo.DamageInfo;
		SafeZoneIndicator->OnRep_CurrentDamageInfo();

		auto OldPhase = SafeZoneIndicator->CurrentPhase;
		SafeZoneIndicator->CurrentPhase = NewSafeZonePhase;
		SafeZoneIndicator->OnRep_CurrentPhase();

		SafeZoneIndicator->OnSafeZonePhaseChanged.Process();

		auto& SafeZoneState = *(uint8_t*)(__int64(&SafeZoneIndicator->FutureReplicator) - 0x4);
		SafeZoneState = 2;
		bool bInitial = OldPhase <= 0;

		SafeZoneIndicator->OnSafeZoneStateChange(2, bInitial);
		SafeZoneIndicator->SafezoneStateChangedDelegate.Process(SafeZoneIndicator, 2);

		SetGamePhaseStep(EAthenaGamePhaseStep::StormHolding);
	}
}


uint64_t Reset_ = 0;
void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Tick()
{
	auto Time = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

	static bool gettingReady = false;
	if (!gettingReady)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && WarmupEarlyCountdownDuration != -1 && WarmupEarlyCountdownDuration < Time)
		{
			gettingReady = true;

			SetGamePhaseStep(EAthenaGamePhaseStep::GetReady);
			return;
		}
	}

	static bool startedBus = false;
	if (gettingReady && !startedBus)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && WarmupCountdownEndTime != -1 && WarmupCountdownEndTime < Time)
		{
			startedBus = true;

			auto Aircraft = Aircrafts_GameState[0].Get();

			//if (Aircraft)
			{
				auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

				if (GameState->MapInfo->FlightInfos.Num() > 0)
				{
					TArray<AFortAthenaAircraft*> Aircrafts;
					auto& FlightInfo = GameState->MapInfo->FlightInfos[0];
					auto Aircraft = AFortAthenaAircraft::SpawnAircraft(UWorld::GetWorld(), GameState->MapInfo->AircraftClass, FlightInfo);

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

					Aircrafts.Add(Aircraft);
					SetAircrafts(Aircrafts);
					OnRep_Aircrafts();
				}


				SetGamePhase(EAthenaGamePhase::Aircraft);
				SetGamePhaseStep(EAthenaGamePhaseStep::BusLocked);
				return;
			}
		}
	}

	static bool busUnlocked = false;
	if (startedBus && !busUnlocked)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && Aircrafts_GameState[0].Get() && Aircrafts_GameState[0]->DropStartTime < Time)
		{
			busUnlocked = true;

			bAircraftIsLocked = false;
			SetGamePhaseStep(EAthenaGamePhaseStep::BusFlying);
			return;
		}
	}

	static bool startedForming = false;
	if (startedBus && !startedForming)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && Aircrafts_GameState[0].Get() && Aircrafts_GameState[0]->DropEndTime != -1 && Aircrafts_GameState[0]->DropEndTime < Time)
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
			return;
		}
	}

	static bool finishedFlight = false;
	if (busUnlocked && !finishedFlight)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && Aircrafts_GameState[0].Get() && Aircrafts_GameState[0]->FlightEndTime != -1 && Aircrafts_GameState[0]->FlightEndTime < Time)
		{
			finishedFlight = true;
			auto Aircraft = Aircrafts_GameState[0].Get();

			Aircraft->K2_DestroyActor();
			Aircrafts_GameState.Clear();
			Aircrafts_GameMode.Clear();
			return;
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
			return;
		}
	}

	static bool bUpdatedPhase = false;
	if (formedZone)
	{
		bool bStartedNewPhase = false;
		if (!bUpdatedPhase && SafeZoneIndicator->SafeZoneStartShrinkTime < Time)
		{
			bUpdatedPhase = true;
			SetGamePhaseStep(EAthenaGamePhaseStep::StormShrinking);
		}
		else if (SafeZoneIndicator->SafeZoneFinishShrinkTime < Time)
		{
			bStartedNewPhase = true;
			StartNewSafeZonePhase(SafeZoneIndicator->CurrentPhase + 1);
			bUpdatedPhase = false;
		}

		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		static auto ZoneEffect = FindObject<UClass>(L"/Game/Athena/SafeZone/GE_OutsideSafeZoneDamage.GE_OutsideSafeZoneDamage_C");

		for (auto& UncastedPlayer : GameMode->AlivePlayers)
		{
			auto Player = (AFortPlayerControllerAthena*)UncastedPlayer;
			if (auto Pawn = Player->MyFortPawn) 
			{
				bool bInZone = IsInCurrentSafeZone(Player->MyFortPawn->K2_GetActorLocation(), true);
				
				if (Pawn->bIsInsideSafeZone != bInZone || Pawn->bIsInAnyStorm != !bInZone)
				{
					//printf("Pawn %s new storm status: %s\n", Pawn->Name.ToString().c_str(), bInZone ? "true" : "false");
					Pawn->bIsInAnyStorm = !bInZone;
					Pawn->OnRep_IsInAnyStorm();
					Pawn->bIsInsideSafeZone = bInZone;
					Pawn->OnRep_IsInsideSafeZone();

					auto AbilitySystemComponent = Player->PlayerState->AbilitySystemComponent;
					for (int i = 0; i < AbilitySystemComponent->ActiveGameplayEffects.GameplayEffects_Internal.Num(); i++)
					{
						auto& Effect = AbilitySystemComponent->ActiveGameplayEffects.GameplayEffects_Internal.Get(i, FActiveGameplayEffect::Size());

						//printf("%s %s\n", Effect.Spec.Def->Name.ToString().c_str(), Effect.Spec.Def->Class->Name.ToString().c_str());
						if (Effect.Spec.Def->Class == ZoneEffect)
						{
							auto Handle = *(FActiveGameplayEffectHandle*)(__int64(&Effect) + 0xc);


							AbilitySystemComponent->SetActiveGameplayEffectLevel(Handle, SafeZoneIndicator->CurrentPhase);

							AbilitySystemComponent->UpdateActiveGameplayEffectSetByCallerMagnitude(Handle, FGameplayTag(UKismetStringLibrary::Conv_StringToName(FString(L"SetByCaller.StormCampingDamage"))), 1);
							//printf("found\n");
							break;
						}
					}
				}

			}
		}
	}
}

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Hook()
{
	if (!GetDefaultObj())
		return;

	Reset_ = FindReset();
	SetGamePhase_ = FindSetGamePhase();

	Utils::Hook(FindHandleMatchHasStarted(), HandleMatchHasStarted, HandleMatchHasStartedOG);
}