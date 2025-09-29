#include "pch.h"
#include "../Public/FortGameModeAthena.h"
#include "../Public/LevelStreamingDynamic.h"
#include "../../Erbium/Public/Finders.h"
#include "../../Engine/Public/NetDriver.h"
#include "../../Engine/Public/AbilitySystemComponent.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortKismetLibrary.h"
#include "../../Engine/Public/CurveTable.h"
#include "../Public/FortSafeZoneIndicator.h"
#include "../../Engine/Public/DataTableFunctionLibrary.h"
#include "../../Erbium/Public/Configuration.h"

void AFortGameModeAthena::ReadyToStartMatch_(UObject* Context, FFrame& Stack, bool* Ret) 
{
    Stack.IncrementCode();

    auto GameMode = Context->Cast<AFortGameModeAthena>();
    if (!GameMode) {
        *Ret = callOGWithRet(((AFortGameModeAthena*)Context), Stack.GetCurrentNativeFunction(), ReadyToStartMatch);
        return;
    }

    auto GameState = GameMode->GameState;
    static auto Playlist = Utils::FindObject<UFortPlaylistAthena>(FConfiguration::Playlist);

    if (GameMode->CurrentPlaylistId == -1)
    {
        // if u listen before setting playlist it behaves the same as using proper listening iirc
        auto World = UWorld::GetWorld();
        auto Engine = UEngine::GetEngine();
        auto NetDriverName = UKismetStringLibrary::Conv_StringToName(FString(L"GameNetDriver"));

        UNetDriver* NetDriver = nullptr;
        if (VersionInfo.FortniteVersion >= 16.00)
        {
            void* WorldCtx = ((void * (*)(UEngine*, UWorld*)) FindGetWorldContext())(Engine, World);
            World->NetDriver = NetDriver = ((UNetDriver * (*)(UEngine*, void*, FName)) FindCreateNetDriverWorldContext())(Engine, WorldCtx, NetDriverName);
        }
        else 
        {
            NetDriver = ((UNetDriver * (*)(UEngine*, UWorld*, FName)) FindCreateNetDriver())(Engine, World, NetDriverName);
        }

        NetDriver->NetDriverName = NetDriverName;
        NetDriver->World = World;

        for (int i = 0; i < World->LevelCollections.Num(); i++)
        {
            auto& LevelCollection = World->LevelCollections.Get(i, FLevelCollection::Size());

            LevelCollection.NetDriver = NetDriver;
        }

        auto URL = (FURL*)malloc(FURL::Size());
        __stosb((PBYTE)URL, 0, FURL::Size());
        URL->Port = 7777;

        auto InitListen = (bool (*)(UNetDriver*, UWorld*, FURL*, bool, FString&)) FindInitListen();
        auto SetWorld = (void (*)(UNetDriver*, UWorld*)) FindSetWorld();

        FString Err;
        if (InitListen(NetDriver, World, URL, false, Err))
        {
            SetWorld(NetDriver, World);
        }
        else
        {
            printf("Failed to listen!");
        }
        free(URL);

        GameMode->WarmupRequiredPlayerCount = 1;

        if (Playlist)
        {
            printf("Playlist: %s\n", Playlist->Name.ToString().c_str());
            if (GameState->HasCurrentPlaylistInfo())
            {
                if (VersionInfo.EngineVersion >= 4.27)
                    Playlist->GarbageCollectionFrequency = 9999999999999999.f; // 4.27 needs a different GC disable method
                GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
                GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
                GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
                GameState->CurrentPlaylistInfo.MarkArrayDirty();
                GameState->OnRep_CurrentPlaylistInfo();
            }
            else if (GameState->HasCurrentPlaylistData())
            {
                GameState->CurrentPlaylistData = Playlist;
                GameState->OnRep_CurrentPlaylistData();
            }

            GameState->CurrentPlaylistId = GameMode->CurrentPlaylistId = Playlist->PlaylistId;
            if (GameMode->HasCurrentPlaylistName())
                GameMode->CurrentPlaylistName = Playlist->PlaylistName;

            if (GameMode->GameSession->HasMaxPlayers())
                GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;



            if (Playlist->HasAdditionalLevels())
                for (auto& Level : Playlist->AdditionalLevels)
                {
                    bool Success = false;
                    printf("Level: %s\n", Level.Get()->Name.ToString().c_str());
                    ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString(), nullptr);
                    auto level = (FAdditionalLevelStreamed*)malloc(FAdditionalLevelStreamed::Size());
                    __stosb((PBYTE)level, 0, FAdditionalLevelStreamed::Size());
                    level->bIsServerOnly = false;
                    level->LevelName = Level.ObjectID.AssetPathName;
                    if (Success)
                        GameState->AdditionalPlaylistLevelsStreamed.Add(*level, FAdditionalLevelStreamed::Size());
                    free(level);
                }

            if (Playlist->HasAdditionalLevelsServerOnly())
                for (auto& Level : Playlist->AdditionalLevelsServerOnly)
                {
                    bool Success = false;
                    printf("Level: %s\n", Level.Get()->Name.ToString().c_str());
                    ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString(), nullptr);
                    auto level = (FAdditionalLevelStreamed*)malloc(FAdditionalLevelStreamed::Size());
                    __stosb((PBYTE)level, 0, FAdditionalLevelStreamed::Size());
                    level->bIsServerOnly = true;
                    level->LevelName = Level.ObjectID.AssetPathName;
                    if (Success)
                        GameState->AdditionalPlaylistLevelsStreamed.Add(*level, FAdditionalLevelStreamed::Size());
                    free(level);
                }
            if (GameState->HasAdditionalPlaylistLevelsStreamed())
                GameState->OnRep_AdditionalPlaylistLevelsStreamed();
        }
        else
        {
            GameState->CurrentPlaylistId = GameMode->CurrentPlaylistId = 0;
        }

        *Ret = false;
        return;
    }

    if (!GameMode->bWorldIsReady)
    {
        static auto WarmupStartClass = FindClass("FortPlayerStartWarmup");
        auto Starts = Utils::GetAll(WarmupStartClass);
        auto StartsNum = Starts.Num();
        Starts.Free();
        if (StartsNum == 0 || !GameState->MapInfo)
        {
            *Ret = false;
            return;
        }
        GameState->OnRep_CurrentPlaylistInfo(); 

        if (VersionInfo.EngineVersion >= 4.27)
        {
            GameMode->bDisableGCOnServerDuringMatch = true;
            GameMode->bPlaylistHotfixChangedGCDisabling = true;
        }

        if (VersionInfo.EngineVersion >= 5.0)
        {
            GameState->DefaultParachuteDeployTraceForGroundDistance = 10000;
        }

        if (VersionInfo.FortniteVersion >= 18)
        {
            // fix storm damage bug
            UCurveTable* AthenaGameDataTable = GameMode->HasAthenaGameDataTable() ? GameMode->AthenaGameDataTable : GameState->AthenaGameDataTable;

            if (AthenaGameDataTable)
            {
                static FName DefaultSafeZoneDamageName = UKismetStringLibrary::Conv_StringToName(FString(L"Default.SafeZone.Damage"));

                for (const auto& [RowName, RowPtr] : AthenaGameDataTable->RowMap)
                {
                    if (RowName != DefaultSafeZoneDamageName)
                        continue;

                    FSimpleCurve* Row = (FSimpleCurve*)RowPtr;

                    if (!Row)
                        continue;

                    for (int i = 0; i < Row->Keys.Num(); i++)
                    {
                        auto& Key = Row->Keys.Get(i, FSimpleCurveKey::Size());

                        if (Key.Time == 0.f)
                            Key.Value = 0.f;
                    }

                    auto NewKey = (FSimpleCurveKey*) malloc(FSimpleCurveKey::Size());
                    NewKey->Time = 1.f;
                    NewKey->Value = 0.01f;
                    Row->Keys.AddAt(*NewKey, 1, FSimpleCurveKey::Size());
                }
            }

        }

        char buffer[67];
        sprintf_s(buffer, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Joinable" : (VersionInfo.FortniteVersion >= 5.00 ? "Erbium (FN %.2f, UE %.2f): Joinable" : "Erbium (FN %.1f, UE %.2f): Joinable"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
        SetConsoleTitleA(buffer);
        GameMode->bWorldIsReady = true;
    }

    *Ret = VersionInfo.EngineVersion < 4.24 ? callOGWithRet(GameMode, Stack.GetCurrentNativeFunction(), ReadyToStartMatch) : GameMode->AlivePlayers.Num() >= GameMode->WarmupRequiredPlayerCount;
    return;
}

auto SpawnDefaultPawnForIdx = 0;

void AFortGameModeAthena::SpawnDefaultPawnFor(UObject* Context, FFrame& Stack, AActor** Ret) 
{
    AFortPlayerControllerAthena* NewPlayer;
    AActor* StartSpot;
    Stack.StepCompiledIn(&NewPlayer);
    Stack.StepCompiledIn(&StartSpot);
    Stack.IncrementCode();
    auto GameMode = (AFortGameModeAthena*)Context;
    // they only stripped it on athena for some reason
    static auto FortGMSpawnDefaultPawnFor = (AFortPlayerPawnAthena * (*)(AFortGameModeAthena*, AFortPlayerControllerAthena*, AActor*)) DefaultObjImpl("FortGameMode")->Vft[SpawnDefaultPawnForIdx];
    auto Pawn = FortGMSpawnDefaultPawnFor(GameMode, NewPlayer, StartSpot);

    //auto Transform = StartSpot->GetTransform();
    //auto Pawn = GameMode->SpawnDefaultPawnAtTransform(NewPlayer, Transform);

    auto Num = NewPlayer->WorldInventory->Inventory.ReplicatedEntries.Num();

    if (Num == 0)
    {
        static auto SmartItemDefClass = FindClass("FortSmartBuildingItemDefinition");
        static bool HasCosmeticLoadoutPC = NewPlayer->HasCosmeticLoadoutPC();
        if (HasCosmeticLoadoutPC)
            NewPlayer->WorldInventory->GiveItem(NewPlayer->CosmeticLoadoutPC.Pickaxe->WeaponDefinition);
        else
            NewPlayer->WorldInventory->GiveItem(NewPlayer->CustomizationLoadout.Pickaxe->WeaponDefinition);

        for (int i = 0; i < GameMode->StartingItems.Num(); i++)
        {
            auto& StartingItem = GameMode->StartingItems.Get(i, FItemAndCount::Size());

            if (StartingItem.Count && (!SmartItemDefClass || !StartingItem.Item->IsA(SmartItemDefClass)))
            {
                NewPlayer->WorldInventory->GiveItem(StartingItem.Item, StartingItem.Count);
            }
        }

        static auto AbilitySet = VersionInfo.FortniteVersion >= 8.30 ? Utils::FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer") : Utils::FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_DefaultPlayer.GAS_DefaultPlayer");
        NewPlayer->PlayerState->AbilitySystemComponent->GiveAbilitySet(AbilitySet);

        static auto ApplyCharacterCustomization = FindApplyCharacterCustomization();

        if (ApplyCharacterCustomization)
            ((void (*)(AActor*, AFortPlayerPawnAthena*)) ApplyCharacterCustomization)(NewPlayer->PlayerState, Pawn);
        else
        {
            //UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(NewPlayer->PlayerState);
        }
    }
    else {
        //NewPlayer->WorldInventory->Inventory.ReplicatedEntries.ResetNum();
        //NewPlayer->WorldInventory->Inventory.ItemInstances.ResetNum();
        static auto AmmoClass = FindClass("FortAmmoItemDefinition");
        static auto ResourceClass = FindClass("FortResourceItemDefinition");
        for (int i = 0; i < NewPlayer->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            auto& Entry = NewPlayer->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

            if (AFortInventory::IsPrimaryQuickbar(Entry.ItemDefinition) || Entry.ItemDefinition->IsA(AmmoClass) || Entry.ItemDefinition->IsA(ResourceClass))
            {
                NewPlayer->WorldInventory->Inventory.ReplicatedEntries.Remove(i, FFortItemEntry::Size());
                i--;
            }
        }

        for (int i = 0; i < NewPlayer->WorldInventory->Inventory.ItemInstances.Num(); i++)
        {
            auto& Entry = NewPlayer->WorldInventory->Inventory.ItemInstances[i]->ItemEntry;

            if (AFortInventory::IsPrimaryQuickbar(Entry.ItemDefinition) || Entry.ItemDefinition->IsA(AmmoClass) || Entry.ItemDefinition->IsA(ResourceClass))
            {
                NewPlayer->WorldInventory->Inventory.ItemInstances.Remove(i);
                i--;
            }
        }
        NewPlayer->WorldInventory->Update(nullptr);

        static bool bMatchStarted = false;

        if (!bMatchStarted)
        {
            bMatchStarted = true;

            char buffer[67];
            sprintf_s(buffer, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Match started" : (VersionInfo.FortniteVersion >= 5.00 ? "Erbium (FN %.2f, UE %.2f): Match started" : "Erbium (FN %.1f, UE %.2f): Match started"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
            SetConsoleTitleA(buffer);
        }
    }

    *Ret = Pawn;
}


void AFortGameModeAthena::HandlePostSafeZonePhaseChanged(AFortGameModeAthena* GameMode, int NewSafeZonePhase_Inp)
{
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    
    auto NewSafeZonePhase = NewSafeZonePhase_Inp >= 0 ? NewSafeZonePhase_Inp : GameMode->SafeZonePhase + 1;

    if (VersionInfo.FortniteVersion >= 13.00)
    {
        static auto DefinitionOffset = GameState->MapInfo->GetOffset("SafeZoneDefinition");
        __int64 SafeZoneDefinition = __int64(GameState->MapInfo) + DefinitionOffset;

        static auto DurationsOffset = 0;
        if (DurationsOffset == 0)
        {
            DurationsOffset = 0x258;

            if (VersionInfo.FortniteVersion >= 18)
                DurationsOffset = 0x248;
            else if (VersionInfo.FortniteVersion < 15.20)
                DurationsOffset = 0x1f8;
        }

        TArray<float>& Durations = *(TArray<float>*)(SafeZoneDefinition + DurationsOffset);
        TArray<float>& HoldDurations = *(TArray<float>*)(SafeZoneDefinition + DurationsOffset - 0x10);


        auto DurationSum = 0.f;
        for (auto& _v : Durations)
            DurationSum += _v;

        if (DurationSum == 0)
        {
            auto GameData = GameMode->HasAthenaGameDataTable() ? GameMode->AthenaGameDataTable : GameState->AthenaGameDataTable;

            auto ShrinkTime = UKismetStringLibrary::Conv_StringToName(FString(L"Default.SafeZone.ShrinkTime"));
            auto HoldTime = UKismetStringLibrary::Conv_StringToName(FString(L"Default.SafeZone.WaitTime"));

            for (int i = 0; i < Durations.Num(); i++)
            {
                UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, ShrinkTime, (float)i, nullptr, &Durations[i], FString());
            }
            for (int i = 0; i < HoldDurations.Num(); i++)
            {
                UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, HoldTime, (float)i, nullptr, &HoldDurations[i], FString());
            }
        }

        auto Duration = Durations[NewSafeZonePhase];
        auto HoldDuration = HoldDurations[NewSafeZonePhase];

        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + HoldDuration;
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
    }


    return HandlePostSafeZonePhaseChangedOG(GameMode, NewSafeZonePhase_Inp);
}


void AFortGameModeAthena::HandleStartingNewPlayer_(UObject* Context, FFrame& Stack) {
    AFortPlayerControllerAthena* NewPlayer;
    Stack.StepCompiledIn(&NewPlayer);
    Stack.IncrementCode();
    auto GameMode = (AFortGameModeAthena*)Context;
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)NewPlayer->PlayerState;

    PlayerState->SquadId = PlayerState->TeamIndex - 3;
    PlayerState->OnRep_SquadId();

    if (GameState->HasGameMemberInfoArray())
    {
        auto Member = (FGameMemberInfo*)malloc(FGameMemberInfo::Size());
        __stosb((PBYTE)Member, 0, FGameMemberInfo::Size());

        Member->MostRecentArrayReplicationKey = -1;
        Member->ReplicationID = -1;
        Member->ReplicationKey = -1;
        Member->TeamIndex = PlayerState->TeamIndex;
        Member->SquadId = PlayerState->SquadId;
        Member->MemberUniqueId = PlayerState->UniqueId;

        GameState->GameMemberInfoArray.Members.Add(*Member, FGameMemberInfo::Size());
        GameState->GameMemberInfoArray.MarkItemDirty(*Member);

        free(Member);
    }

    return callOG(GameMode, Stack.GetCurrentNativeFunction(), HandleStartingNewPlayer, NewPlayer);
}


uint8_t AFortGameModeAthena::PickTeam(AFortGameModeAthena* GameMode, uint8_t PreferredTeam, AFortPlayerControllerAthena* Controller) {
    uint8_t ret = CurrentTeam;

    if (++PlayersOnCurTeam >= GameMode->GameState->CurrentPlaylistInfo.BasePlaylist->MaxSquadSize) {
        CurrentTeam++;
        PlayersOnCurTeam = 0;
    }

    return ret;
}

void AFortGameModeAthena::Hook()
{
    auto spdf = GetDefaultObj()->GetFunction("SpawnDefaultPawnFor");
    SpawnDefaultPawnForIdx = spdf->GetVTableIndex();
    Utils::ExecHook(GetDefaultObj()->GetFunction("ReadyToStartMatch"), ReadyToStartMatch_, ReadyToStartMatch_OG);
    Utils::ExecHook(spdf, SpawnDefaultPawnFor);
    Utils::Hook(FindHandlePostSafeZonePhaseChanged(), HandlePostSafeZonePhaseChanged, HandlePostSafeZonePhaseChangedOG);
    Utils::ExecHook(GetDefaultObj()->GetFunction("HandleStartingNewPlayer"), HandleStartingNewPlayer_, HandleStartingNewPlayer_OG);
    Utils::Hook(FindPickTeam(), PickTeam, PickTeamOG);
}