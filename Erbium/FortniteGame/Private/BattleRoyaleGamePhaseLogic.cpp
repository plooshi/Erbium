#include "pch.h"
#include "../Public/BattleRoyaleGamePhaseLogic.h"
#include "../../Erbium/Public/Configuration.h"
#include "../../Erbium/Public/GUI.h"

uint64_t SetGamePhase_ = 0;
void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::SetGamePhase(EAthenaGamePhase GamePhase)
{
    auto SetGamePhaseInternal = (void (*)(UFortGameStateComponent_BattleRoyaleGamePhaseLogic*, EAthenaGamePhase))SetGamePhase_;

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

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::HandleMatchHasStarted(AFortGameMode* GameMode)
{
    HandleMatchHasStartedOG(GameMode);
    auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameMode);

    if (!bSkipWarmup)
    {
        auto Time = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        auto WarmupDuration = 60.f;

        GamePhaseLogic->WarmupCountdownStartTime = Time;
        GamePhaseLogic->WarmupCountdownEndTime = Time + WarmupDuration;
        GamePhaseLogic->WarmupCountdownDuration = 10.f;
        GamePhaseLogic->WarmupEarlyCountdownDuration = WarmupDuration - 10.f;

        GamePhaseLogic->SetGamePhase(EAthenaGamePhase::Warmup);
        GamePhaseLogic->SetGamePhaseStep(EAthenaGamePhaseStep::Warmup);
    }
    else
    {
        printf("[GamePhaseLogic] Skipping warmup\n");
        GamePhaseLogic->StartAircraftPhase();
    }
}

AFortSafeZoneIndicator* UFortGameStateComponent_BattleRoyaleGamePhaseLogic::SetupSafeZoneIndicator()
{
    // thanks heliato

    if (!this->SafeZoneIndicator)
    {
        AFortSafeZoneIndicator* SafeZoneIndicator = UWorld::SpawnActor<AFortSafeZoneIndicator>(SafeZoneIndicatorClass, FVector{});

        if (SafeZoneIndicator)
        {
            auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
            FFortSafeZoneDefinition& SafeZoneDefinition = GameState->MapInfo->SafeZoneDefinition;
            float SafeZoneCount = SafeZoneDefinition.Count.Evaluate();

            auto& Array = SafeZoneIndicator->SafeZonePhases;

            if (Array.IsValid())
                Array.Free();

            const float Time = (float)UGameplayStatics::GetTimeSeconds(GameState);

            for (float i = 0; i < SafeZoneCount; i++)
            {
                auto PhaseInfo = (FFortSafeZonePhaseInfo*)malloc(FFortSafeZonePhaseInfo::Size());
                memset((PBYTE)PhaseInfo, 0, FFortSafeZonePhaseInfo::Size());

                PhaseInfo->Radius = SafeZoneDefinition.Radius.Evaluate(i);
                PhaseInfo->WaitTime = SafeZoneDefinition.WaitTime.Evaluate(i);
                PhaseInfo->ShrinkTime = SafeZoneDefinition.ShrinkTime.Evaluate(i);
                PhaseInfo->PlayerCap = (int)SafeZoneDefinition.PlayerCapSolo.Evaluate(i);

                UDataTableFunctionLibrary::EvaluateCurveTableRow(GameState->AthenaGameDataTable, FName(L"Default.SafeZone.Damage"), i, nullptr, &PhaseInfo->DamageInfo.Damage, FString());
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

                PhaseInfo->Center = SafeZoneLocations[(int)i];

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

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::StartNewSafeZonePhase(int NewSafeZonePhase, bool bInitial)
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

        SafeZoneIndicator->SafeZoneStartShrinkTime = FConfiguration::bLateGame && FConfiguration::bLateGameLongZone ? 676767.f : TimeSeconds + PhaseInfo.WaitTime;
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

        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        if (!bInitial)
            for (auto& UncastedPlayer : GameMode->AlivePlayers)
            {
                auto PlayerController = (AFortPlayerControllerAthena*)UncastedPlayer;

                PlayerController->GetQuestManager(1)->SendStatEvent(PlayerController, EFortQuestObjectiveStatEvent::GetStormPhase(), 1, false);
            }
    }
}

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::StartAircraftPhase()
{
    static auto GamePhaseOffset = this->GetOffset("GamePhase");
    auto& _GamePhase = *(EAthenaGamePhase*)(__int64(this) + GamePhaseOffset);

    if (_GamePhase >= EAthenaGamePhase::Aircraft)
        return;

    auto Time = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

    auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
    auto Playlist = VersionInfo.FortniteVersion >= 3.5 && GameMode->HasWarmupRequiredPlayerCount()
                        ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData)
                        : nullptr;

    if constexpr (FConfiguration::WebhookURL && *FConfiguration::WebhookURL)
    {
        auto curl = curl_easy_init();

        curl_easy_setopt(curl, CURLOPT_URL, FConfiguration::WebhookURL);
        curl_slist* headers = curl_slist_append(NULL, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        char version[6];

        sprintf_s(version, VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "%.2f" : "%.1f", VersionInfo.FortniteVersion);

        auto payload = UEAllocatedString("{\"embeds\": [{\"title\": \"Match has started!\", \"fields\": [{\"name\":\"Version\",\"value\":\"") + version + "\"}, {\"name\":\"Playlist\",\"value\":\"" +
                       (Playlist ? Playlist->PlaylistName.ToString() : "Playlist_DefaultSolo") + "\"},{\"name\":\"Players\",\"value\":\"" + std::to_string(GameMode->AlivePlayers.Num()).c_str() +
                       "\"}], \"color\": " +
                       "\"7237230\", \"footer\": {\"text\":\"Erbium\", "
                       "\"icon_url\":\"https://cdn.discordapp.com/attachments/1341168629378584698/1436803905119064105/"
                       "L0WnFa.png.png?ex=6910ef69&is=690f9de9&hm=01a0888b46647959b38ee58df322048ab49e2a5a678e52d4502d9c5e3978d805&\"}, \"timestamp\":\"" +
                       iso8601() + "\"}] }";

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

        curl_easy_perform(curl);

        curl_easy_cleanup(curl);
    }
    GUI::gsStatus = StartedMatch;
    sprintf_s(GUI::windowTitle,
              VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Match started"
                                               : (VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "Erbium (FN %.2f, UE %.2f): Match started" : "Erbium (FN %.1f, UE %.2f): Match started"),
              VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
    SetConsoleTitleA(GUI::windowTitle);

    if (FConfiguration::bJoinInProgress || (Playlist && Playlist->bAllowJoinInProgress))
        *(bool*)(uint64_t(&GameMode->WarmupRequiredPlayerCount) - 4) = false;

    if (bSkipAircraft)
    {
        printf("[GamePhaseLogic] Skipping aircraft\n");
        SetGamePhase(EAthenaGamePhase::SafeZones);
        SetGamePhaseStep(EAthenaGamePhaseStep::StormForming);

        return;
    }

    auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

    if (GameState->MapInfo->FlightInfos.Num() > 0)
    {
        // TArray<TWeakObjectPtr<AFortAthenaAircraft>> Aircrafts;
        auto& FlightInfo = GameState->MapInfo->FlightInfos[0];

        if (FConfiguration::bLateGame)
        {
            /*if (VersionInfo.FortniteVersion < 16)
            {
                    GameState->GamePhase = 4;
                    GameState->GamePhaseStep = 7;
                    GameState->OnRep_GamePhase(3);
            }*/

            GameState->DefaultParachuteDeployTraceForGroundDistance = 2500.f;

            FVector Loc = SafeZoneLocations[FConfiguration::LateGameZone + 2];
            Loc.Z = 17500.f;

            FlightInfo.FlightSpeed = 0.f;

            FlightInfo.FlightStartLocation = Loc;

            FlightInfo.TimeTillFlightEnd = 7.f;
            FlightInfo.TimeTillDropEnd = 7.f;
            FlightInfo.TimeTillDropStart = 0.f;
            // GameState->bAircraftIsLocked = false;
            // GameState->SafeZonesStartTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 8.f;
        }

        if (!GameState->MapInfo->AircraftClass.Get())
            return;
        auto Aircraft = AFortAthenaAircraft::SpawnAircraft(UWorld::GetWorld(), GameState->MapInfo->AircraftClass, FlightInfo);

        if (!Aircraft)
            return;

        Aircraft->FlightElapsedTime = 0;
        Aircraft->DropStartTime = (float)Time + FlightInfo.TimeTillDropStart;
        Aircraft->DropEndTime = (float)Time + FlightInfo.TimeTillDropEnd;
        Aircraft->FlightStartTime = (float)Time;
        Aircraft->FlightEndTime = (float)Time + FlightInfo.TimeTillFlightEnd;
        Aircraft->ReplicatedFlightTimestamp = (float)Time;
        bAircraftIsLocked = true;

        for (auto& Player__Uncasted : ((AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers)
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
            auto Reset = (void (*)(AFortPlayerControllerAthena*))FindReset();
            Reset(Player);
            Player->ClientGotoState(FName(L"Spectating"));
        }

        Aircrafts_GameMode.Add(Aircraft);
        Aircrafts_GameState.Add(Aircraft);
        // SetAircrafts(Aircrafts);
        // OnRep_Aircrafts();
    }

    SetGamePhase(EAthenaGamePhase::Aircraft);
    SetGamePhaseStep(EAthenaGamePhaseStep::BusLocked);
}

uint64_t Reset_ = 0;
uint64_t IsInsideSafeZone = 0;
void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Tick()
{
    auto Time = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
    static auto GamePhaseOffset = this->GetOffset("GamePhase");
    auto& _GamePhase = *(EAthenaGamePhase*)(__int64(this) + GamePhaseOffset);

    static bool finishedFlight = false;
    if (!bSkipAircraft)
    {
        if (_GamePhase <= EAthenaGamePhase::Warmup)
        {
            static bool gettingReady = false;
            if (!bStartAircraft && !gettingReady)
            {
                if (((AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && WarmupEarlyCountdownDuration != -1 && WarmupEarlyCountdownDuration < Time)
                {
                    gettingReady = true;

                    SetGamePhaseStep(EAthenaGamePhaseStep::GetReady);
                    return;
                }
            }

            if (bStartAircraft || gettingReady)
            {
                if (bStartAircraft || (((AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && WarmupCountdownEndTime != -1 && WarmupCountdownEndTime < Time))
                {
                    StartAircraftPhase();

                    return;
                }
            }
        }

        if (_GamePhase == EAthenaGamePhase::Aircraft)
        {
            static bool busUnlocked = false;
            if (!busUnlocked)
            {
                if (((AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && Aircrafts_GameState[0].Get() && Aircrafts_GameState[0]->DropStartTime < Time)
                {
                    busUnlocked = true;

                    bAircraftIsLocked = false;
                    SetGamePhaseStep(EAthenaGamePhaseStep::BusFlying);
                    return;
                }
            }

            static bool startedForming = false;
            if (!startedForming)
            {
                if (((AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && Aircrafts_GameState[0].Get() && Aircrafts_GameState[0]->DropEndTime != -1 &&
                    Aircrafts_GameState[0]->DropEndTime < Time)
                {
                    startedForming = true;
                    auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
                    auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;

                    for (auto& Player__Uncasted : GameMode->AlivePlayers)
                    {
                        auto Player = (AFortPlayerControllerAthena*)Player__Uncasted;
                        if (!Player->PlayerState->bIsABot && Player->IsInAircraft())
                        {
                            Player->GetAircraftComponent()->ServerAttemptAircraftJump(FRotator{});
                        }
                    }

                    /*if (bLateGame)
                    {
                            GameState->GamePhase = EAthenaGamePhase::SafeZones;
                            GameState->GamePhaseStep = EAthenaGamePhaseStep::StormHolding;
                            GameState->OnRep_GamePhase(EAthenaGamePhase::Aircraft);
                    }*/
                    if (FConfiguration::bLateGame)
                        SafeZonesStartTime = (float)Time;
                    else
                        SafeZonesStartTime = (float)Time + 60.f;

                    SetGamePhase(EAthenaGamePhase::SafeZones);
                    SetGamePhaseStep(EAthenaGamePhaseStep::StormForming);
                    return;
                }
            }
        }

        if (!finishedFlight)
        {
            if (((AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && Aircrafts_GameState[0].Get() && Aircrafts_GameState[0]->FlightEndTime != -1 &&
                Aircrafts_GameState[0]->FlightEndTime < Time)
            {
                finishedFlight = true;
                auto Aircraft = Aircrafts_GameState[0].Get();

                Aircraft->K2_DestroyActor();
                Aircrafts_GameState.Clear();
                Aircrafts_GameMode.Clear();
                return;
            }
        }
    }
    else
    {
        if (!finishedFlight)
        {
            finishedFlight = true;

            SetGamePhase(EAthenaGamePhase::SafeZones);
            SetGamePhaseStep(EAthenaGamePhaseStep::StormForming);
        }
    }

    if (bEnableZones)
    {
        if (_GamePhase == EAthenaGamePhase::SafeZones)
        {
            static bool formedZone = false;
            if (!bPausedZone && finishedFlight && !formedZone)
            {
                if (((AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && SafeZonesStartTime != -1 && SafeZonesStartTime < Time)
                {
                    formedZone = true;
                    auto SafeZoneIndicator = SetupSafeZoneIndicator();
                    StartNewSafeZonePhase(FConfiguration::bLateGame ? FConfiguration::LateGameZone + 3 : 1, true);
                    return;
                }
            }

            static bool bUpdatedPhase = false;
            if (formedZone && SafeZoneIndicator)
            {
                if (SafeZoneIndicator->SafeZonePhases.IsValidIndex(SafeZoneIndicator->CurrentPhase))
                {
                    bool bStartedNewPhase = false;
                    if (!bPausedZone && !bUpdatedPhase && SafeZoneIndicator->SafeZoneStartShrinkTime < Time)
                    {
                        bUpdatedPhase = true;

                        auto& SafeZoneState = *(uint8_t*)(__int64(&SafeZoneIndicator->FutureReplicator) - 0x4);
                        SafeZoneState = 3;

                        SafeZoneIndicator->OnSafeZoneStateChange(3, false);
                        SafeZoneIndicator->SafezoneStateChangedDelegate.Process(SafeZoneIndicator, 3);

                        SetGamePhaseStep(EAthenaGamePhaseStep::StormShrinking);
                    }
                    else if (!bPausedZone && SafeZoneIndicator->SafeZoneFinishShrinkTime < Time)
                    {
                        bStartedNewPhase = true;

                        if (SafeZoneIndicator->SafeZonePhases.IsValidIndex(SafeZoneIndicator->CurrentPhase + 1))
                        {
                            StartNewSafeZonePhase(SafeZoneIndicator->CurrentPhase + 1);
                            bUpdatedPhase = false;
                        }
                    }
                }

                auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
                static auto ZoneEffect = FindObject<UClass>(L"/Game/Athena/SafeZone/GE_OutsideSafeZoneDamage.GE_OutsideSafeZoneDamage_C");

                for (auto& UncastedPlayer : GameMode->AlivePlayers)
                {
                    auto Player = (AFortPlayerControllerAthena*)UncastedPlayer;
                    if (auto Pawn = Player->MyFortPawn)
                    {
                        bool bInZone = IsInCurrentSafeZone(Player->MyFortPawn->K2_GetActorLocation(), false);

                        if (!IsInCurrentSafeZone__Ptr)
                            bInZone = GameMode->IsInCurrentSafeZone(Player->MyFortPawn->K2_GetActorLocation(), false);

                        if (Pawn->bIsInsideSafeZone != bInZone || Pawn->bIsInAnyStorm != !bInZone)
                        {
                            printf("Pawn %s new storm status: %s\n", Pawn->Name.ToString().c_str(), bInZone ? "true" : "false");
                            Pawn->bIsInAnyStorm = !bInZone;
                            Pawn->OnRep_IsInAnyStorm();
                            Pawn->bIsInsideSafeZone = bInZone;
                            Pawn->OnRep_IsInsideSafeZone();

                            /*auto AbilitySystemComponent = Player->PlayerState->AbilitySystemComponent;
                            if (AbilitySystemComponent)
                            {
                                    for (int i = 0; i < AbilitySystemComponent->ActiveGameplayEffects.GameplayEffects_Internal.Num(); i++)
                                    {
                                            auto& Effect = AbilitySystemComponent->ActiveGameplayEffects.GameplayEffects_Internal.Get(i, FActiveGameplayEffect::Size());

                                            printf("%s %s\n", Effect.Spec.Def->Name.ToString().c_str(), Effect.Spec.Def->Class->Name.ToString().c_str());
                                            if (Effect.Spec.Def->Class == ZoneEffect)
                                            {
                                                    auto Handle = *(FActiveGameplayEffectHandle*)(__int64(&Effect) + 0xc);

                                                    AbilitySystemComponent->SetActiveGameplayEffectLevel(Handle, SafeZoneIndicator->CurrentPhase);

                                                    // 1.f should be max(InStormDamageIncrementValue, 1.f)
                                                    AbilitySystemComponent->UpdateActiveGameplayEffectSetByCallerMagnitude(Handle,
                            FGameplayTag(FName(L"SetByCaller.StormCampingDamage")), 1.f); AbilitySystemComponent->UpdateActiveGameplayEffectSetByCallerMagnitude(Handle,
                            FGameplayTag(FName(L"SetByCaller.StormShieldDamage")), 1.f); printf("found\n"); break;
                                            }
                                    }
                            }*/
                        }
                    }
                }
            }
        }
    }
}

struct FVector4 // Size: 0x20
{
public:
    double X; // 0x0 // Size: 0x8
    double Y; // 0x8 // Size: 0x8
    double Z; // 0x10 // Size: 0x8
    double W; // 0x18 // Size: 0x8
};

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::InitializeSafeZoneLocations()
{
    auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
    auto Playlist = (UFortPlaylistAthena*)GameState->CurrentPlaylistInfo.BasePlaylist;

    auto SafeZoneBlacklist = Playlist->SafeZoneLocationBlacklist.Get();

    if (!SafeZoneBlacklist)
        SafeZoneBlacklist = FindObject<UCurveTable>(L"/Game/Athena/Balance/DataTables/AthenaSafeZoneBlacklist.AthenaSafeZoneBlacklist");

    auto& SZBCurve = (TMap<FName, FRealCurve*>&)SafeZoneBlacklist->RowMap;

    TArray<FVector4> BlacklistLocations;

    for (auto& [Key, Curve] : SZBCurve)
    {
        FSimpleCurve* Row = (FSimpleCurve*)Curve;

        if (!Row)
            continue;

        FVector4 Loc{};

        for (auto& Key : Row->Keys)
        {
            if (Key.Time == 0.f)
                Loc.X = Key.Value;
            else if (Key.Time == 1.f)
                Loc.Y = Key.Value;
            else if (Key.Time == 2.f)
                Loc.Z = Key.Value;
            else if (Key.Time == 3.f)
                Loc.W = Key.Value;
        }

        if (Loc.X == 0 && Loc.Y == 0 && Loc.Z == 0 && Loc.W == 0)
            continue;

        BlacklistLocations.Add(Loc);
    }

    auto ZeroVector = FVector(0, 0, 0);
    auto SafeZoneCount = (float)Playlist->LastSafeZoneIndex;

    if (SafeZoneCount == -1)
        SafeZoneCount = GameState->MapInfo->SafeZoneDefinition.Count.Evaluate(0.f);
    else
        SafeZoneCount++;

    auto Center = GameState->MapInfo->GetMapCenter();

    SafeZoneLocations.Clear();
    SafeZoneLocations.Reserve((int)SafeZoneCount);

    for (int i = (int)(SafeZoneCount - 1); i >= 0; i--)
    {
        auto Params = GameState->MapInfo->ConstructSafeZoneLocationParams(i, Center, i == SafeZoneCount - 1 ? ZeroVector : SafeZoneLocations[i + 1], i == SafeZoneCount - 1, 0);

        auto Location = GameState->MapInfo->PickSafeZoneLocation(Params, BlacklistLocations);

        SafeZoneLocations[i] = Location;
    }
}

void UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Hook()
{
    if (!GetDefaultObj())
        return;

    Reset_ = FindReset();
    SetGamePhase_ = FindSetGamePhase();

    Hooking::Hook(FindHandleMatchHasStarted(), HandleMatchHasStarted, HandleMatchHasStartedOG);
}
