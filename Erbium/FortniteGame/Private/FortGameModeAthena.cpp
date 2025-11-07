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
#include "../Public/FortLootPackage.h"
#include "../Public/BuildingFoundation.h"
#include "../../Erbium/Public/LateGame.h"
#include "../Public/BuildingItemCollectorActor.h"
#include "../../Erbium/Public/GUI.h"
#include <random>
#include "../../Erbium/Public/Misc.h"
#include "../../Erbium/Public/Events.h"
#include "../Public/BattleRoyaleGamePhaseLogic.h"

void ShowFoundation(const ABuildingFoundation* Foundation)
{
    if (!Foundation) return;

    /*Foundation->StreamingData.BoundingBox = Foundation->StreamingBoundingBox;
    Foundation->StreamingData.FoundationLocation = Foundation->GetTransform().Translation;
    Foundation->SetDynamicFoundationEnabled(true);*/
    //Foundation->SetDynamicFoundationTransform(Foundation->GetTransform());

    if (Foundation->HasDynamicFoundationType())
        Foundation->DynamicFoundationType = 0;
    if (Foundation->HasbServerStreamedInLevel())
    {
        Foundation->bServerStreamedInLevel = true;
        Foundation->OnRep_ServerStreamedInLevel();
    }
    if (Foundation->HasDynamicFoundationRepData())
    {
        Foundation->DynamicFoundationRepData.EnabledState = 1;
        Foundation->OnRep_DynamicFoundationRepData();
    }
    if (Foundation->HasFoundationEnabledState())
        Foundation->FoundationEnabledState = 1;

    Foundation->SetDynamicFoundationEnabled(true);
}

bool bIsLargeTeamGame = false;

void SetupPlaylist(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState)
{
    static auto Playlist = FindObject<UFortPlaylistAthena>(FConfiguration::Playlist);

    if (Playlist)
    {
        if (FConfiguration::bForceRespawns)
        {
            Playlist->bRespawnInAir = true;
            Playlist->RespawnHeight.Curve.CurveTable = nullptr;
            Playlist->RespawnHeight.Curve.RowName = FName();
            Playlist->RespawnHeight.Value = 20000;
            Playlist->RespawnTime.Curve.CurveTable = nullptr;
            Playlist->RespawnTime.Curve.RowName = FName();
            Playlist->RespawnTime.Value = 3;
            Playlist->RespawnType = 1; // InfiniteRespawns
            Playlist->bAllowJoinInProgress = true;
            if (Playlist->HasbForceRespawnLocationInsideOfVolume())
                Playlist->bForceRespawnLocationInsideOfVolume = true;
        }
        if (Playlist->HasGarbageCollectionFrequency())
            Playlist->GarbageCollectionFrequency = 9999999999999999.f; // easier than hooking collectgarbage
        if (GameMode->HasPlaylistHotfixOriginalGCFrequency())
            GameMode->PlaylistHotfixOriginalGCFrequency = 9999999999999999.f;
        if (GameMode->HasbDisableGCOnServerDuringMatch())
            GameMode->bDisableGCOnServerDuringMatch = true;
        if (GameMode->HasbPlaylistHotfixChangedGCDisabling())
            GameMode->bPlaylistHotfixChangedGCDisabling = true;
        if (GameState->HasCurrentPlaylistInfo())
        {
            //if (VersionInfo.EngineVersion >= 4.27)
            GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
            GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
            GameState->CurrentPlaylistInfo.MarkArrayDirty();
            GameState->OnRep_CurrentPlaylistInfo();
        }
        else if (GameState->HasCurrentPlaylistData())
        {
            GameState->CurrentPlaylistData = Playlist;
            GameState->OnRep_CurrentPlaylistData();
        }

        GameMode->CurrentPlaylistId = Playlist->PlaylistId;
        if (GameState->HasCurrentPlaylistId())
            GameState->CurrentPlaylistId = Playlist->PlaylistId;
        if (GameMode->HasCurrentPlaylistName())
            GameMode->CurrentPlaylistName = Playlist->PlaylistName;

        if (GameMode->GameSession->HasMaxPlayers())
            GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;


        if (GameState->HasAirCraftBehavior() && Playlist->HasAirCraftBehavior())
            GameState->AirCraftBehavior = Playlist->AirCraftBehavior;
        if (GameState->HasCachedSafeZoneStartUp() && Playlist->HasSafeZoneStartUp())
            GameState->CachedSafeZoneStartUp = Playlist->SafeZoneStartUp;

        if (GameMode->HasbEnableDBNO())
            GameMode->bEnableDBNO = Playlist->MaxSquadSize > 1;

        bIsLargeTeamGame = Playlist->bIsLargeTeamGame;

        // misc C1 poi things
        if (VersionInfo.FortniteVersion >= 6 && VersionInfo.FortniteVersion < 7)
        {
            if (VersionInfo.FortniteVersion > 6.10)
                ShowFoundation(VersionInfo.FortniteVersion <= 6.21 ? FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Lake1") : FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Lake2"));
            else
                ShowFoundation(FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_StreamingTest12"));

            ShowFoundation(VersionInfo.FortniteVersion <= 6.10 ? FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_StreamingTest13") : FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_FloatingIsland"));

            auto IslandScripting = TUObjectArray::FindFirstObject("BP_IslandScripting_C");
            auto UpdateMapOffset = IslandScripting->GetOffset("UpdateMap");
            if (UpdateMapOffset != -1)
            {
                *(bool*)(__int64(IslandScripting) + UpdateMapOffset) = true;
                IslandScripting->ProcessEvent(IslandScripting->GetFunction("OnRep_UpdateMap"), nullptr);
            }
        }
        else if (VersionInfo.FortniteVersion >= 7 && VersionInfo.FortniteVersion < 8)
        {
            ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_POI_25x36"));
            ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.ShopsNew"));
        }
        else if (VersionInfo.FortniteVersion >= 8 && VersionInfo.FortniteVersion < 10)
            ShowFoundation(FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_POI_50x53_Volcano"));
        else if (VersionInfo.FortniteVersion >= 10.20 && VersionInfo.FortniteVersion < 11)
            ShowFoundation(FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_POI_50x53_Volcano"));

        if (VersionInfo.FortniteVersion >= 7 && VersionInfo.FortniteVersion <= 10)
            ShowFoundation(FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.SLAB_2"));
        else if (VersionInfo.EngineVersion == 4.23) // rest of S10
            ShowFoundation(FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.SLAB_4"));

        bool bEvent = false;
        if (Playlist->HasGameplayTagContainer())
        {
            for (int i = 0; i < Playlist->GameplayTagContainer.GameplayTags.Num(); i++)
            {
                auto& PlaylistTag = Playlist->GameplayTagContainer.GameplayTags.Get(i, FGameplayTag::Size());

                if (PlaylistTag.TagName.ToString() == "Athena.Playlist.SpecialEvent")
                {
                    bEvent = true;
                    if (VersionInfo.FortniteVersion == 7.30)
                        ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.PleasentParkFestivus"));
                    else if (VersionInfo.FortniteVersion == 12.41)
                    {
                        ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.LF_Athena_POI_19x19_2"));
                        ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head6_18"));
                        ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head5_14"));
                        ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head3_8"));
                        ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head_2"));
                        ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head4_11"));
                    }

                    break;
                }
            }
        }

        if (VersionInfo.FortniteVersion == 7.30 && !bEvent)
            ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.PleasentParkDefault"));


        auto AdditionalPlaylistLevelsStreamed__Off = GameState->GetOffset("AdditionalPlaylistLevelsStreamed");
        auto AdditionalLevelStruct = FAdditionalLevelStreamed::StaticStruct();
        if (Playlist->HasAdditionalLevels())
            for (auto& Level : Playlist->AdditionalLevels)
            {
                bool Success = false;
                ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString(), nullptr);
                if (AdditionalLevelStruct)
                {
                    auto level = (FAdditionalLevelStreamed*)malloc(FAdditionalLevelStreamed::Size());
                    memset((PBYTE)level, 0, FAdditionalLevelStreamed::Size());
                    level->bIsServerOnly = false;
                    level->LevelName = Level.ObjectID.AssetPathName;
                    if (Success)
                        GameState->AdditionalPlaylistLevelsStreamed.Add(*level, FAdditionalLevelStreamed::Size());
                    free(level);
                }
                else
                    GetFromOffset<TArray<FName>>(GameState, AdditionalPlaylistLevelsStreamed__Off).Add(Level.ObjectID.AssetPathName);
            }

        if (Playlist->HasAdditionalLevelsServerOnly())
            for (auto& Level : Playlist->AdditionalLevelsServerOnly)
            {
                bool Success = false;
                ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), Level, FVector(), FRotator(), &Success, FString(), nullptr);

                if (AdditionalLevelStruct)
                {
                    auto level = (FAdditionalLevelStreamed*)malloc(FAdditionalLevelStreamed::Size());
                    memset((PBYTE)level, 0, FAdditionalLevelStreamed::Size());
                    level->bIsServerOnly = true;
                    level->LevelName = Level.ObjectID.AssetPathName;
                    if (Success)
                        GameState->AdditionalPlaylistLevelsStreamed.Add(*level, FAdditionalLevelStreamed::Size());
                    free(level);
                }
                else
                    GetFromOffset<TArray<FName>>(GameState, AdditionalPlaylistLevelsStreamed__Off).Add(Level.ObjectID.AssetPathName);
            }
        if (GameState->HasAdditionalPlaylistLevelsStreamed())
            GameState->OnRep_AdditionalPlaylistLevelsStreamed();
    }
    else
    {
        GameState->CurrentPlaylistId = GameMode->CurrentPlaylistId = 0;
    }
}


void (*VendWobble__FinishedFuncOG)(UObject* Context, FFrame& Stack);
void VendWobble__FinishedFunc(UObject* Context, FFrame& Stack)
{
    auto CollectorActor = (ABuildingItemCollectorActor*)Context;
    auto PlayerController = CollectorActor->ControllingPlayer;

    if (!PlayerController)
        return VendWobble__FinishedFuncOG(Context, Stack);

    auto Collection = CollectorActor->ItemCollections.Search([&](FCollectorUnitInfo& Coll)
        {
            return Coll.InputItem == CollectorActor->ClientPausedActiveInputItem;
        }, FCollectorUnitInfo::Size());

    if (!Collection)
        return VendWobble__FinishedFuncOG(Context, Stack);

    CollectorActor->ClientPausedActiveInputItem = nullptr;

    float Cost = Collection->InputCount.Evaluate();

    auto VMLoc = CollectorActor->K2_GetActorLocation();
    auto& SpawnLocation = CollectorActor->LootSpawnLocation;
    auto Loc = VMLoc + (CollectorActor->GetActorForwardVector() * SpawnLocation.X) + (CollectorActor->GetActorRightVector() * SpawnLocation.Y) + (CollectorActor->GetActorUpVector() * SpawnLocation.Z);

    for (int i = 0; i < Collection->OutputItemEntry.Num(); i++)
    {
        auto& Item = Collection->OutputItemEntry.Get(i, FFortItemEntry::Size());

        AFortInventory::SpawnPickup(Loc, Item);
        if (CollectorActor->HasPickupSpawned())
            CollectorActor->PickupSpawned.Process();
    }

    /*if (Cost == 0)
    {
        CollectorActor->DoVendDeath();
        CollectorActor->K2_DestroyActor();
    }*/

    return VendWobble__FinishedFuncOG(Context, Stack);
}

std::map<int, float> WeightMap;
float Sum = 0;
float Weight;
float TotalWeight;

void AFortGameModeAthena::ReadyToStartMatch_(UObject* Context, FFrame& Stack, bool* Ret)
{
    Stack.IncrementCode();

    auto GameMode = Context->Cast<AFortGameModeAthena>();
    if (!GameMode)
    {
        *Ret = callOGWithRet(((AFortGameModeAthena*)Context), Stack.GetCurrentNativeFunction(), ReadyToStartMatch);
        return;
    }

    auto GameState = GameMode->GameState;

    if (GameMode->WarmupRequiredPlayerCount != 1)
    {
        // credit mariki
        if (UWorld::GetWorld()->HasServerStreamingLevelsVisibility())
            UWorld::GetWorld()->ServerStreamingLevelsVisibility = UWorld::SpawnActor<AServerStreamingLevelsVisibility>(FVector{}, {});

        // if u listen before setting playlist it behaves the same as using proper listening iirc
        auto World = UWorld::GetWorld();
        auto Engine = UEngine::GetEngine();
        auto NetDriverName = FName(L"GameNetDriver");

        UNetDriver* NetDriver = nullptr;
        if (VersionInfo.FortniteVersion >= 16.00)
        {
            void* WorldCtx = ((void* (*)(UEngine*, UWorld*)) FindGetWorldContext())(Engine, World);
            World->NetDriver = NetDriver = ((UNetDriver * (*)(UEngine*, void*, FName, int)) FindCreateNetDriverWorldContext())(Engine, WorldCtx, NetDriverName, 0);
        }
        else
            World->NetDriver = NetDriver = ((UNetDriver * (*)(UEngine*, UWorld*, FName)) FindCreateNetDriver())(Engine, World, NetDriverName);
        if (VersionInfo.FortniteVersion >= 20)
            NetDriver->NetServerMaxTickRate = 30;

        NetDriver->NetDriverName = NetDriverName;
        NetDriver->World = World;

        if (VersionInfo.EngineVersion >= 5.3 && FConfiguration::bEnableIris)
        {
            *(bool*)(__int64(&NetDriver->ReplicationDriver) + 0x11) = true;
        }

        NetDriver->NetDriverName = NetDriverName;
        NetDriver->World = World;

        for (int i = 0; i < World->LevelCollections.Num(); i++)
        {
            auto& LevelCollection = World->LevelCollections.Get(i, FLevelCollection::Size());

            LevelCollection.NetDriver = NetDriver;
        }

        auto URL = (FURL*)malloc(FURL::Size());
        memset((PBYTE)URL, 0, FURL::Size());
        URL->Port = 7777;

        auto InitListen = (bool (*)(UNetDriver*, UWorld*, FURL*, bool, FString&)) FindInitListen();
        auto SetWorld = (void (*)(UNetDriver*, UWorld*)) FindSetWorld();

        SetWorld(NetDriver, World);
        FString Err;
        if (InitListen(NetDriver, World, URL, false, Err))
            SetWorld(NetDriver, World);
        else
            printf("Failed to listen!");

        free(URL);

        GameMode->WarmupRequiredPlayerCount = 1;

        if (VersionInfo.FortniteVersion >= 4.0)
            SetupPlaylist(GameMode, GameState);
        
        if (VersionInfo.EngineVersion >= 4.27)
        {
            auto MeshNetworkSubsystem = TUObjectArray::FindFirstObject("MeshNetworkSubsystem");

            if (MeshNetworkSubsystem)
                *(uint8_t*)(__int64(MeshNetworkSubsystem) + MeshNetworkSubsystem->GetOffset("NodeType")) = 2;
        }

        *Ret = false;
        return;
    }

    if (!GameMode->bWorldIsReady)
    {
        static auto WarmupStartClass = FindClass("PlayerStart");
        auto Starts = Utils::GetAll(WarmupStartClass);
        auto StartsNum = Starts.Num();
        Starts.Free();

        static bool bHasMapInfo = true;

        if (wcsstr(FConfiguration::Playlist, L"/MoleGame/Playlists/Playlist_MoleGame"))
            bHasMapInfo = false;

        if (StartsNum == 0 || (bHasMapInfo && !GameState->MapInfo))
        {
            *Ret = false;
            return;
        }

        if (VersionInfo.FortniteVersion >= 3.5 && VersionInfo.FortniteVersion <= 4.0)
            SetupPlaylist(GameMode, GameState);
        else if (VersionInfo.EngineVersion >= 4.22 && VersionInfo.EngineVersion < 4.26)
            GameState->OnRep_CurrentPlaylistInfo();

        if (VersionInfo.FortniteVersion >= 25.20)
        {
            auto InitializeFlightPath = (void(*)(AFortAthenaMapInfo*, AFortGameStateAthena*, UFortGameStateComponent_BattleRoyaleGamePhaseLogic*, bool, double, float, float)) FindInitializeFlightPath();
            if (InitializeFlightPath)
                InitializeFlightPath(GameState->MapInfo, GameState, UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameState), false, 0.f, 0.f, 360.f);
            UFortGameStateComponent_BattleRoyaleGamePhaseLogic::GenerateStormCircles(GameState->MapInfo);
        }

        static auto Playlist = FindObject<UFortPlaylistAthena>(FConfiguration::Playlist);

        if (Playlist && Playlist->HasbSkipWarmup())
            UFortGameStateComponent_BattleRoyaleGamePhaseLogic::bSkipWarmup = Playlist->bSkipWarmup;
        if (Playlist && Playlist->HasbSkipAircraft())
            UFortGameStateComponent_BattleRoyaleGamePhaseLogic::bSkipAircraft = Playlist->bSkipAircraft;

        if (Playlist && Playlist->HasGameplayTagContainer())
        {

            for (int i = 0; i < Playlist->GameplayTagContainer.GameplayTags.Num(); i++)
            {
                auto& PlaylistTag = Playlist->GameplayTagContainer.GameplayTags.Get(i, FGameplayTag::Size());

                if (PlaylistTag.TagName.ToString() == "Athena.Playlist.SpecialEvent")
                {
                    for (auto& Event : Events::EventsArray)
                    {
                        if (Event.EventVersion != VersionInfo.FortniteVersion)
                            continue;

                        UObject* LoaderObject = nullptr;
                        if (Event.LoaderClass)
                            if (const UClass* LoaderClass = FindObject<UClass>(Event.LoaderClass))
                            {
                                auto AllLoaders = Utils::GetAll(LoaderClass);
                                LoaderObject = AllLoaders.Num() > 0 ? AllLoaders[0] : nullptr;
                            }

                        if (Event.LoaderFuncPath != nullptr && LoaderObject)
                            if (const UFunction* LoaderFunction = FindObject<UFunction>(Event.LoaderFuncPath))
                            {
                                int Param = 1;
                                LoaderObject->ProcessEvent(const_cast<UFunction*>(LoaderFunction), &Param);
                                printf("[Events] Loaded event level!\n");
                            }
                            else
                                printf("[Events] Failed to load event level!\n");

                        if (GameMode->HasSafeZoneLocations())
                            GameMode->SafeZoneLocations.Free();
                        else
                            UFortGameStateComponent_BattleRoyaleGamePhaseLogic::bEnableZones = false;
                        break;
                    }

                    break;
                }
            }
        }

        if (VersionInfo.FortniteVersion >= 20)
        {
            auto TacticalSprintAbility = FindObject<UFortAbilitySet>(L"/TacticalSprintGame/Gameplay/AS_TacticalSprint.AS_TacticalSprint");

            if (!TacticalSprintAbility)
                TacticalSprintAbility = FindObject<UFortAbilitySet>(L"/TacticalSprint/Gameplay/AS_TacticalSprint.AS_TacticalSprint");
            AbilitySets.Add(TacticalSprintAbility);
            AbilitySets.Add(FindObject<UFortAbilitySet>(L"/Ascender/Gameplay/Ascender/AS_Ascender.AS_Ascender"));
        }

        auto AbilitySet = VersionInfo.FortniteVersion >= 8.30 ? FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer") : FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_DefaultPlayer.GAS_DefaultPlayer");
        AbilitySets.Add(AbilitySet);

        if (Playlist && Playlist->HasModifierList())
            for (int i = 0; i < Playlist->ModifierList.Num(); i++)
            {
                auto Modifier = Playlist->ModifierList.Get(i, FSoftObjectPtr::Size()).Get();

                if (!Modifier)
                    continue;

                for (int j = 0; j < Modifier->PersistentAbilitySets.Num(); j++)
                {
                    auto& DeliveryInfo = Modifier->PersistentAbilitySets.Get(j, FFortAbilitySetDeliveryInfo::Size());

                    if (!DeliveryInfo.DeliveryRequirements.bApplyToPlayerPawns)
                        continue;

                    for (int k = 0; k < DeliveryInfo.AbilitySets.Num(); k++)
                    {
                        auto AbilitySet = DeliveryInfo.AbilitySets.Get(k, FSoftObjectPtr::Size()).Get();

                        AbilitySets.Add(AbilitySet);
                    }
                }
            }

        auto AddToTierData = [&](const UDataTable* Table, UEAllocatedMap<int32, FFortLootTierData*>& TempArr)
            {
                if (!Table)
                    return;

                Table->AddToRoot();
                if (VersionInfo.FortniteVersion >= 20)
                {
                    if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
                        for (auto& ParentTable : CompositeTable->ParentTables)
                            for (auto& [Key, Val] : *(TMap<int32, FFortLootTierData*>*) (__int64(ParentTable) + 0x30))
                                TempArr[Key] = Val;

                    for (auto& [Key, Val] : *(TMap<int32, FFortLootTierData*>*) (__int64(Table) + 0x30))
                        TempArr[Key] = Val;
                }
                else
                {
                    if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
                        for (auto& ParentTable : CompositeTable->ParentTables)
                            for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) ParentTable->RowMap)
                                TempArr[Key.ComparisonIndex] = Val;

                    for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) Table->RowMap)
                        TempArr[Key.ComparisonIndex] = Val;
                }
            };

        auto AddToPackages = [&](const UDataTable* Table, UEAllocatedMap<int32, FFortLootPackageData*>& TempArr)
            {
                if (!Table)
                    return;

                Table->AddToRoot();
                if (VersionInfo.FortniteVersion >= 20)
                {
                    if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
                        for (auto& ParentTable : CompositeTable->ParentTables)
                            for (auto& [Key, Val] : *(TMap<int32, FFortLootPackageData*>*) (__int64(ParentTable) + 0x30))
                                TempArr[Key] = Val;

                    for (auto& [Key, Val] : *(TMap<int32, FFortLootPackageData*>*) (__int64(Table) + 0x30))
                        TempArr[Key] = Val;
                }
                else
                {
                    if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
                        for (auto& ParentTable : CompositeTable->ParentTables)
                            for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>) ParentTable->RowMap)
                                TempArr[Key.ComparisonIndex] = Val;

                    for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>) Table->RowMap)
                        TempArr[Key.ComparisonIndex] = Val;
                }
            };

        UEAllocatedMap<int32, FFortLootTierData*> LootTierDataTempArr;
        auto LootTierData = Playlist ? Playlist->LootTierData.Get() : nullptr;
        if (!LootTierData)
            LootTierData = FindObject<UDataTable>(L"/Game/Items/Datatables/AthenaLootTierData_Client.AthenaLootTierData_Client");
        if (LootTierData)
            AddToTierData(LootTierData, LootTierDataTempArr);

        for (auto& [_, Val] : LootTierDataTempArr)
            TierDataMap[Val->TierGroup.ComparisonIndex].Add(Val);
        //    TierDataAllGroups.Add(Val);

        UEAllocatedMap<int32, FFortLootPackageData*> LootPackageTempArr;
        auto LootPackages = Playlist ? Playlist->LootPackages.Get() : nullptr;
        if (!LootPackages)
            LootPackages = FindObject<UDataTable>(L"/Game/Items/Datatables/AthenaLootPackages_Client.AthenaLootPackages_Client");
        if (LootPackages)
            AddToPackages(LootPackages, LootPackageTempArr);

        for (auto& [_, Val] : LootPackageTempArr)
            LootPackageMap[Val->LootPackageID.ComparisonIndex].Add(Val);

        auto GameFeatureDataClass = FindClass("FortGameFeatureData");
        if (GameFeatureDataClass)
            for (int i = 0; i < TUObjectArray::Num(); i++)
            {
                auto Object = TUObjectArray::GetObjectByIndex(i);

                if (!Object || !Object->Class || Object->IsDefaultObject())
                    continue;

                if (Object->IsA(GameFeatureDataClass))
                {
                    static auto DefaultLootTableDataOffset = Object->GetOffset("DefaultLootTableData");
                    static auto PlaylistOverrideLootTableDataOffset = Object->GetOffset("PlaylistOverrideLootTableData");

                    auto& LootTableData = GetFromOffset<FFortGameFeatureLootTableData>(Object, DefaultLootTableDataOffset);
                    auto& LootTableDataUE53 = GetFromOffset<FFortGameFeatureLootTableData_UE53>(Object, DefaultLootTableDataOffset);
                    auto& PlaylistOverrideLootTableData = GetFromOffset<TMap<FGameplayTag, FFortGameFeatureLootTableData>>(Object, PlaylistOverrideLootTableDataOffset);
                    auto& PlaylistOverrideLootTableDataLWC = GetFromOffset<TMap<int32, FFortGameFeatureLootTableData>>(Object, PlaylistOverrideLootTableDataOffset);
                    auto& PlaylistOverrideLootTableDataUE53 = GetFromOffset<TMap<int32, FFortGameFeatureLootTableData_UE53>>(Object, PlaylistOverrideLootTableDataOffset);
                    auto LTDFeatureData = VersionInfo.EngineVersion >= 5.3 ? LootTableDataUE53.LootTierData.Get() : LootTableData.LootTierData.Get();
                    auto LootPackageData = VersionInfo.EngineVersion >= 5.3 ? LootTableDataUE53.LootPackageData.Get() : LootTableData.LootPackageData.Get();

                    if (LTDFeatureData)
                    {
                        UEAllocatedMap<int32, FFortLootTierData*> LTDTempData;

                        AddToTierData(LTDFeatureData, LTDTempData);

                        if (Playlist)
                        {
                            if (VersionInfo.EngineVersion >= 5.3)
                            {
                                /*for (auto& Tag : Playlist->GameplayTagContainer.GameplayTags)
                                    for (auto& Override : PlaylistOverrideLootTableDataUE52)
                                        if (Tag.TagName.ComparisonIndex == Override.First)
                                            AddToTierData(Override.Second.LootTierData.Get(), LTDTempData);*/
                            }
                            else if (VersionInfo.FortniteVersion < 20.00)
                            {
                                for (auto& Tag : Playlist->GameplayTagContainer.GameplayTags)
                                    for (auto& Override : PlaylistOverrideLootTableData)
                                        if (Tag.TagName == Override.First.TagName)
                                            AddToTierData(Override.Second.LootTierData.Get(), LTDTempData);
                            }
                            else
                                for (auto& Tag : Playlist->GameplayTagContainer.GameplayTags)
                                    for (auto& Override : PlaylistOverrideLootTableDataLWC)
                                        if (Tag.TagName.ComparisonIndex == Override.First)
                                            AddToTierData(Override.Second.LootTierData.Get(), LTDTempData);
                        }

                        //for (auto& [_, Val] : LTDTempData)
                        //    TierDataAllGroups.Add(Val);

                        for (auto& [_, Val] : LTDTempData)
                            TierDataMap[Val->TierGroup.ComparisonIndex].Add(Val);
                    }

                    if (LootPackageData)
                    {
                        UEAllocatedMap<int32, FFortLootPackageData*> LPTempData;

                        AddToPackages(LootPackageData, LPTempData);

                        if (Playlist)
                        {
                            if (VersionInfo.EngineVersion >= 5.3)
                            {
                                /*for (auto& Tag : Playlist->GameplayTagContainer.GameplayTags)
                                    for (auto& Override : PlaylistOverrideLootTableDataUE52)
                                        if (Tag.TagName.ComparisonIndex == Override.First)
                                            AddToPackages(Override.Second.LootPackageData.Get(), LPTempData);*/
                            }
                            else if (VersionInfo.FortniteVersion < 20.00)
                            {
                                for (auto& Tag : Playlist->GameplayTagContainer.GameplayTags)
                                    for (auto& Override : PlaylistOverrideLootTableData)
                                        if (Tag.TagName == Override.First.TagName)
                                            AddToPackages(Override.Second.LootPackageData.Get(), LPTempData);
                            }
                            else
                                for (auto& Tag : Playlist->GameplayTagContainer.GameplayTags)
                                    for (auto& Override : PlaylistOverrideLootTableDataLWC)
                                        if (Tag.TagName.ComparisonIndex == Override.First)
                                            AddToPackages(Override.Second.LootPackageData.Get(), LPTempData);
                        }


                        for (auto& [_, Val] : LPTempData)
                            LootPackageMap[Val->LootPackageID.ComparisonIndex].Add(Val);
                    }
                }
            }

        if (floor(VersionInfo.FortniteVersion) != 20)
        {
            UFortLootPackage::SpawnFloorLootForContainer(FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C"));
            UFortLootPackage::SpawnFloorLootForContainer(FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C"));
        }

        auto ConsumableSpawners = Utils::GetAll<ABGAConsumableSpawner>();

        for (auto& Spawner : ConsumableSpawners)
            UFortLootPackage::SpawnConsumableActor(Spawner);

        if (VersionInfo.EngineVersion >= 4.27)
        {
            GameState->DefaultParachuteDeployTraceForGroundDistance = 10000;
        }

        if (VersionInfo.FortniteVersion >= 18 && VersionInfo.FortniteVersion < 25.20)
        {
            // fix storm damage bug
            UCurveTable* AthenaGameDataTable = GameMode->HasAthenaGameDataTable() ? GameMode->AthenaGameDataTable : GameState->AthenaGameDataTable;

            if (AthenaGameDataTable)
            {
                static FName DefaultSafeZoneDamageName = FName(L"Default.SafeZone.Damage");

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

                    auto NewKey = (FSimpleCurveKey*)malloc(FSimpleCurveKey::Size());
                    NewKey->Time = 1.f;
                    NewKey->Value = 0.01f;
                    Row->Keys.AddAt(*NewKey, 1, FSimpleCurveKey::Size());
                }
            }
        }

        const UObject* BattleBusDef = nullptr;
        const UClass* SupplyDropClass = nullptr;
        if (VersionInfo.FortniteVersion == 18.40)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_HeadbandBus.BBID_HeadbandBus");
        else if (VersionInfo.FortniteVersion == 1.11 || VersionInfo.FortniteVersion == 7.30 || VersionInfo.FortniteVersion == 11.31 || VersionInfo.FortniteVersion == 15.10 || VersionInfo.FortniteVersion == 19.01 || VersionInfo.FortniteVersion == 23.10 || VersionInfo.FortniteVersion == 28.01)
        {
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_WinterBus.BBID_WinterBus");
            SupplyDropClass = FindObject<UClass>(L"/Game/Athena/SupplyDrops/AthenaSupplyDrop_Holiday.AthenaSupplyDrop_Holiday_C");
        }
        else if (VersionInfo.FortniteVersion == 5.10 || VersionInfo.FortniteVersion == 9.41 || VersionInfo.FortniteVersion == 14.20 || VersionInfo.FortniteVersion == 18.00)
        {
            if (VersionInfo.FortniteVersion == 5.10)
                BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus.BBID_BirthdayBus");
            else if (VersionInfo.FortniteVersion == 9.41)
                BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus2nd.BBID_BirthdayBus2nd");
            else if (VersionInfo.FortniteVersion == 14.20)
                BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus3rd.BBID_BirthdayBus3rd");
            else
                BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus4th.BBID_BirthdayBus4th");

            SupplyDropClass = FindObject<UClass>(L"/Game/Athena/SupplyDrops/AthenaSupplyDrop_BDay.AthenaSupplyDrop_BDay_C");
        }
        else if (VersionInfo.FortniteVersion == 6.20 || VersionInfo.FortniteVersion == 6.21 || VersionInfo.FortniteVersion == 11.10 || VersionInfo.FortniteVersion == 14.40 || VersionInfo.FortniteVersion == 18.21)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_HalloweenBus.BBID_HalloweenBus");
        else if (VersionInfo.FortniteVersion == 14.30)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BusUpgrade1.BBID_BusUpgrade1");
        else if (VersionInfo.FortniteVersion == 14.50)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BusUpgrade2.BBID_BusUpgrade2");
        else if (VersionInfo.FortniteVersion == 14.60)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BusUpgrade3.BBID_BusUpgrade3");
        else if (VersionInfo.FortniteVersion >= 12.30 && VersionInfo.FortniteVersion <= 12.61)
        {
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_DonutBus.BBID_DonutBus");
            BattleBusDef = FindObject<UClass>(L"/Game/Athena/SupplyDrops/AthenaSupplyDrop_Donut.AthenaSupplyDrop_Donut_C");
        }
        else if (VersionInfo.FortniteVersion == 9.30)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_WorldCupBus.BBID_WorldCupBus");
        else if (VersionInfo.FortniteVersion == 21.00)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_CelebrationBus.BBID_CelebrationBus");
        else if (std::floor(VersionInfo.FortniteVersion) == 27)
			BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_DefaultBus.BBID_DefaultBus");

        if (BattleBusDef)
        {
            GameState->DefaultBattleBus = BattleBusDef;

            for (auto& Aircraft : Utils::GetAll<AFortAthenaAircraft>())
            {
                Aircraft->DefaultBusSkin = BattleBusDef;

                if (Aircraft->SpawnedCosmeticActor)
                {
                    static auto Offset = Aircraft->SpawnedCosmeticActor->GetOffset("ActiveSkin");

                    GetFromOffset<const UObject*>(Aircraft->SpawnedCosmeticActor, Offset) = BattleBusDef;
                }
            }
        }

        if (SupplyDropClass && GameState->MapInfo)
            for (auto& Info : GameState->MapInfo->SupplyDropInfoList)
                Info->SupplyDropClass = SupplyDropClass;


        if (VersionInfo.FortniteVersion >= 3.4 && GameState->MapInfo)
        {
            GameData = Playlist ? Playlist->GameData : nullptr;
            if (!GameData)
                GameData = FindObject<UCurveTable>(L"/Game/Athena/Balance/DataTables/AthenaGameData.AthenaGameData");

            for (int i = 0; i < 6; i++)
            {
                float Weight;
                UDataTableFunctionLibrary::EvaluateCurveTableRow(GameState->MapInfo->VendingMachineRarityCount.Curve.CurveTable, GameState->MapInfo->VendingMachineRarityCount.Curve.RowName, (float)i, nullptr, &Weight, FString());

                WeightMap[i] = Weight;
                Sum += Weight;
            }

            UDataTableFunctionLibrary::EvaluateCurveTableRow(GameState->MapInfo->VendingMachineRarityCount.Curve.CurveTable, GameState->MapInfo->VendingMachineRarityCount.Curve.RowName, 0.f, nullptr, &Weight, FString());

            TotalWeight = std::accumulate(WeightMap.begin(), WeightMap.end(), 0.0f, [&](float acc, const std::pair<int, float>& p)
                { return acc + p.second; });
        }

        GameMode->DefaultPawnClass = FindObject<UClass>(L"/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C");

        if (VersionInfo.EngineVersion == 4.16)
        {
            auto sRef = Memcury::Scanner::FindStringRef(L"CollectGarbageInternal() is flushing async loading").Get();
            uint64_t CollectGarbage = 0;

            if (sRef)
            {
                for (int i = 0; i < 1000; i++)
                {
                    auto Ptr = (uint8_t*)(sRef - i);

                    if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
                    {
                        CollectGarbage = uint64_t(Ptr);
                        break;
                    }
                    else if (*Ptr == 0x40 && *(Ptr + 1) == 0x55)
                    {
                        CollectGarbage = uint64_t(Ptr);
                        break;
                    }
                    else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
                    {
                        CollectGarbage = uint64_t(Ptr);
                        break;
                    }
                }

                Utils::Patch<uint8_t>(CollectGarbage, 0xC3);
            }
        }

        GUI::gsStatus = 1;
        sprintf_s(GUI::windowTitle, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Joinable" : (VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "Erbium (FN %.2f, UE %.2f): Joinable" : "Erbium (FN %.1f, UE %.2f): Joinable"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
        SetConsoleTitleA(GUI::windowTitle);
        GameMode->bWorldIsReady = true;
    }

    if (VersionInfo.EngineVersion >= 4.24)
    {
        int ReadyPlayers = 0;
        auto PlayerList = Utils::GetAll<AFortPlayerControllerAthena>();

        for (auto& PlayerController : PlayerList)
        {
            auto PlayerState = PlayerController->PlayerState;

            if (!PlayerState->bIsSpectator && PlayerController->bReadyToStartMatch)
                ReadyPlayers++;
        }

        PlayerList.Free();

        auto VolumeManager = GameState->VolumeManager;
        TArray<FPlaylistStreamedLevelData>& AdditionalPlaylistLevels = *(TArray<FPlaylistStreamedLevelData>*) (__int64(GameState) + GameState->GetOffset("AdditionalPlaylistLevelsStreamed") - 0x10);

        bool bAllLevelsFinishedStreaming = true;
        for (int i = 0; i < AdditionalPlaylistLevels.Num(); i++)
        {
            auto& AdditionalPlaylistLevel = AdditionalPlaylistLevels.Get(i, FPlaylistStreamedLevelData::Size());

            if (!AdditionalPlaylistLevel.bIsFinishedStreaming || !AdditionalPlaylistLevel.StreamingLevel || !AdditionalPlaylistLevel.StreamingLevel->LoadedLevel->bIsVisible)
            {
                bAllLevelsFinishedStreaming = false;
                break;
            }
        }

        static auto WaitingToStart = FName(L"WaitingToStart");
        *Ret = GameMode->bWorldIsReady && GameState->bPlaylistDataIsLoaded && GameMode->MatchState == WaitingToStart && bAllLevelsFinishedStreaming && (!VolumeManager || !VolumeManager->bInSpawningStartup) && ReadyPlayers >= GameMode->WarmupRequiredPlayerCount;
    }
    else
        *Ret = callOGWithRet(GameMode, Stack.GetCurrentNativeFunction(), ReadyToStartMatch);

    if (VersionInfo.FortniteVersion >= 11.00 && VersionInfo.FortniteVersion < 25.20 && !*Ret)
    {
        auto Time = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        auto WarmupDuration = 60.f;

        if (GameState->HasWarmupCountdownEndTime()) // gamephaselogic builds
        {
            GameState->WarmupCountdownStartTime = Time;
            GameState->WarmupCountdownEndTime = Time + WarmupDuration;
            GameMode->WarmupCountdownDuration = WarmupDuration;
            GameMode->WarmupEarlyCountdownDuration = WarmupDuration;
        }
    }
    return;
}

auto SpawnDefaultPawnForIdx = 0;
uint64_t ApplyCharacterCustomization;

void AFortGameModeAthena::SpawnDefaultPawnFor(UObject* Context, FFrame& Stack, AActor** Ret)
{
    AFortPlayerControllerAthena* NewPlayer;
    AActor* StartSpot;
    Stack.StepCompiledIn(&NewPlayer);
    Stack.StepCompiledIn(&StartSpot);
    Stack.IncrementCode();
    auto GameMode = (AFortGameModeAthena*)Context;
    auto GameState = GameMode->GameState;
    auto Num = NewPlayer->WorldInventory->Inventory.ReplicatedEntries.Num();
    // they only stripped it on athena for some reason
    AFortPlayerPawnAthena* Pawn = nullptr;
    static auto FortGMSpawnDefaultPawnFor = (AFortPlayerPawnAthena * (*)(AFortGameModeAthena*, AFortPlayerControllerAthena*, AActor*)) DefaultObjImpl("FortGameMode")->Vft[SpawnDefaultPawnForIdx];
    Pawn = FortGMSpawnDefaultPawnFor(GameMode, NewPlayer, StartSpot);

    //auto Transform = StartSpot->GetTransform();
    //auto Pawn = GameMode->SpawnDefaultPawnAtTransform(NewPlayer, Transform);


    if (Num == 0)
    {
        if (VersionInfo.FortniteVersion <= 1.91 && VersionInfo.FortniteVersion != 1.1 && VersionInfo.FortniteVersion != 1.11)
        {
            static auto HeroCharPartsOffset = NewPlayer->StrongMyHero->GetOffset("CharacterParts");
            auto& HeroCharParts = GetFromOffset<TArray<UObject*>>(NewPlayer->StrongMyHero, HeroCharPartsOffset);
            static auto CharacterPartsOffset = NewPlayer->PlayerState->GetOffset("CharacterParts");
            auto& CharacterParts = GetFromOffset<const UObject * [0x6]>(NewPlayer->PlayerState, CharacterPartsOffset);
            
            if (HeroCharParts.Num() > 0)
            {
                for (auto& Part : HeroCharParts)
                {
                    static auto PartTypeOffset = Part->GetOffset("CharacterPartType");
                    CharacterParts[GetFromOffset<uint8>(Part, PartTypeOffset)] = Part;
                }
            }
            else
            {

                static auto Head = FindObject<UObject>(L"/Game/Characters/CharacterParts/Female/Medium/Heads/F_Med_Head1.F_Med_Head1");
                static auto Body = FindObject<UObject>(L"/Game/Characters/CharacterParts/Female/Medium/Bodies/F_Med_Soldier_01.F_Med_Soldier_01");
                static auto Backpack = FindObject<UObject>(L"/Game/Characters/CharacterParts/Backpacks/NoBackpack.NoBackpack");

                CharacterParts[0] = Head;
                CharacterParts[1] = Body;
                CharacterParts[3] = Backpack;
            }
        }



        if (ApplyCharacterCustomization)
            ((void (*)(AActor*, AFortPlayerPawnAthena*)) ApplyCharacterCustomization)(NewPlayer->PlayerState, Pawn);
        else
        {
            //UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(NewPlayer->PlayerState);
        }

        if (NewPlayer->HasXPComponent())
        {
            if (NewPlayer->PlayerState->HasSeasonLevelUIDisplay())
            {
                NewPlayer->PlayerState->SeasonLevelUIDisplay = NewPlayer->XPComponent->CurrentLevel;
                NewPlayer->PlayerState->OnRep_SeasonLevelUIDisplay();
            }

            if (NewPlayer->XPComponent->HasbRegisteredWithQuestManager())
            {
                NewPlayer->XPComponent->bRegisteredWithQuestManager = true;
                NewPlayer->XPComponent->OnRep_bRegisteredWithQuestManager();
            }
        }

        static bool bFinalSetup = false;
        if (!bFinalSetup)
        {
            bFinalSetup = true;

            if (floor(VersionInfo.FortniteVersion) == 20)
            {
                UFortLootPackage::SpawnFloorLootForContainer(FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C"));
                UFortLootPackage::SpawnFloorLootForContainer(FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C"));
            }

            if (VersionInfo.FortniteVersion > 3.4)
            {
                for (auto& CollectorActor : Utils::GetAll<ABuildingItemCollectorActor>())
                {
                    if (Sum > Weight)
                    {
                    PickNum:
                        auto RandomNum = (float)rand() / (RAND_MAX / TotalWeight);

                        int Rarity = 0;
                        bool found = false;

                        for (auto& Element : WeightMap)
                        {
                            float Weight = Element.second;

                            if (Weight == 0)
                                continue;

                            if (RandomNum <= Weight)
                            {
                                Rarity = Element.first;

                                found = true;
                                break;
                            }

                            RandomNum -= Weight;
                        }

                        if (!found)
                            goto PickNum;

                        if (Rarity == 0)
                        {
                            CollectorActor->K2_DestroyActor();
                            continue;
                        }

                        int AttemptsToGetItem = 0;
                        for (int i = 0; i < CollectorActor->ItemCollections.Num(); i++)
                        {
                            if (AttemptsToGetItem > 10)
                            {
                                AttemptsToGetItem = 0;
                                goto PickNum;
                            }

                            auto& Collection = CollectorActor->ItemCollections.Get(i, FCollectorUnitInfo::Size());

                            if (Collection.bUseDefinedOutputItem)
                                continue;

                            auto LootDrops = UFortLootPackage::ChooseLootForContainer(CollectorActor->DefaultItemLootTierGroupName, Rarity);

                            if (Collection.OutputItemEntry.Num() > 0)
                            {
                                Collection.OutputItemEntry.ResetNum();
                                Collection.OutputItem = nullptr;
                            }

                            for (auto& LootDrop : LootDrops)
                            {
                                if (!Collection.OutputItem && AFortInventory::IsPrimaryQuickbar(LootDrop->ItemDefinition))
                                    Collection.OutputItem = LootDrop->ItemDefinition;

                                Collection.OutputItemEntry.Add(*LootDrop, FFortItemEntry::Size());
                                free(LootDrop);
                            }

                            if (!Collection.OutputItem)
                            {
                                i--;
                                AttemptsToGetItem++;

                                continue;
                            }

                            AttemptsToGetItem = 0;
                        }

                        CollectorActor->StartingGoalLevel = Rarity;
                    }
                    else
                        CollectorActor->K2_DestroyActor();
                }

                Utils::ExecHook((UFunction*)FindObject<UFunction>(L"/Game/Athena/Items/Gameplay/VendingMachine/B_Athena_VendingMachine.B_Athena_VendingMachine_C:VendWobble__FinishedFunc"), VendWobble__FinishedFunc, VendWobble__FinishedFuncOG);
            }
            //Utils::ExecHook((UFunction*)FindObject<UFunction>(L"/Game/Athena/Items/Consumables/Parents/GA_Athena_MedConsumable_Parent.GA_Athena_MedConsumable_Parent_C:Triggered_4C02BFB04B18D9E79F84848FFE6D2C32"), AFortPlayerPawnAthena::Athena_MedConsumable_Triggered, AFortPlayerPawnAthena::Athena_MedConsumable_TriggeredOG);
        }
    }
    else
    {
        //NewPlayer->WorldInventory->Inventory.ReplicatedEntries.ResetNum();
        //NewPlayer->WorldInventory->Inventory.ItemInstances.ResetNum();

        /*for (int i = 0; i < NewPlayer->WorldInventory->Inventory.ItemInstances.Num(); i++)
        {
            auto& Entry = NewPlayer->WorldInventory->Inventory.ItemInstances[i]->ItemEntry;

            if (AFortInventory::IsPrimaryQuickbar(Entry.ItemDefinition) || Entry.ItemDefinition->IsA(AmmoClass) || Entry.ItemDefinition->IsA(ResourceClass))
            {
                NewPlayer->WorldInventory->Inventory.ItemInstances.Remove(i);
                i--;
            }
        }

        NewPlayer->WorldInventory->Update(nullptr);*/
    }

    *Ret = Pawn;
}


void AFortGameModeAthena::HandlePostSafeZonePhaseChanged(AFortGameModeAthena* GameMode, int NewSafeZonePhase_Inp)
{
    if (!GameMode->SafeZoneIndicator)
        return;

    auto NewSafeZonePhase = NewSafeZonePhase_Inp >= 0 ? NewSafeZonePhase_Inp : ((GameMode->HasSafeZonePhase() ? GameMode->SafeZonePhase : GameMode->SafeZoneIndicator->CurrentPhase) + 1);
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    float TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(GameState);

    if (VersionInfo.FortniteVersion >= 21.50)
    {
        if (HandlePostSafeZonePhaseChangedOG)
            HandlePostSafeZonePhaseChangedOG(GameMode, NewSafeZonePhase_Inp);

        return;
    }


    constexpr static std::array<float, 8> LateGameDurations{
        0.f,
        120.f,
        90.f,
        60.f,
        50.f,
        35.f,
        30.f,
        40.f,
    };

    constexpr static std::array<float, 8> LateGameHoldDurations{
        0.f,
        90.f,
        75.f,
        60.f,
        45.f,
        30.f,
        0.f,
        0.f,
    };

    if (VersionInfo.FortniteVersion >= 13.00)
    {
        auto SafeZoneDefinition = &GameState->MapInfo->SafeZoneDefinition;

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


        static bool bSetDurations = false;
        if (!bSetDurations)
        {
            bSetDurations = true;

            auto GameData = GameMode->HasAthenaGameDataTable() ? GameMode->AthenaGameDataTable : GameState->AthenaGameDataTable;

            auto ShrinkTime = FName(L"Default.SafeZone.ShrinkTime");
            auto HoldTime = FName(L"Default.SafeZone.WaitTime");

            for (int i = 0; i < Durations.Num(); i++)
            {
                UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, ShrinkTime, (float)i, nullptr, &Durations[i], FString());
            }
            for (int i = 0; i < HoldDurations.Num(); i++)
            {
                UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, HoldTime, (float)i, nullptr, &HoldDurations[i], FString());
            }
        }

        if (!FConfiguration::bLateGame)
        {
            auto Duration = Durations[NewSafeZonePhase];
            auto HoldDuration = HoldDurations[NewSafeZonePhase];

            GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds + HoldDuration;
            GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
        }
    }

    HandlePostSafeZonePhaseChangedOG(GameMode, NewSafeZonePhase_Inp);

    if (FConfiguration::bLateGame && GameMode->SafeZonePhase > FConfiguration::LateGameZone)
    {
        auto newIdx = GameMode->SafeZonePhase - FConfiguration::LateGameZone + 1;
        auto Duration = LateGameDurations.size() >= newIdx ? 0.f : LateGameDurations[newIdx];
        auto HoldDuration = LateGameHoldDurations.size() >= newIdx ? 0.f : LateGameHoldDurations[newIdx];

        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds + HoldDuration;
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
    }


    if (FConfiguration::bLateGame && GameMode->SafeZonePhase < FConfiguration::LateGameZone)
    {
        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds;
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + 0.15f;
    }
    else if (FConfiguration::bLateGame && GameMode->SafeZonePhase == FConfiguration::LateGameZone)
    {
        auto Duration = LateGameDurations[FConfiguration::LateGameZone];
        auto HoldDuration = LateGameHoldDurations[FConfiguration::LateGameZone];

        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = FConfiguration::bLateGame && FConfiguration::bLateGameLongZone ? 676767.f : TimeSeconds + HoldDuration;
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
    }
}


uint64_t NotifyGameMemberAdded_ = 0;
void AFortGameModeAthena::HandleStartingNewPlayer_(UObject* Context, FFrame& Stack)
{
    AFortPlayerControllerAthena* NewPlayer;
    Stack.StepCompiledIn(&NewPlayer);
    Stack.IncrementCode();
    auto GameMode = (AFortGameModeAthena*)Context;
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)NewPlayer->PlayerState;

    if (VersionInfo.FortniteVersion <= 2.5)
    {
        NewPlayer->QuickBars = UWorld::SpawnActor<AFortQuickBars>(FVector{});
        NewPlayer->QuickBars->SetOwner(NewPlayer);
    }

    if (PlayerState->HasSquadId())
    {
        PlayerState->SquadId = PlayerState->TeamIndex - 3;
        PlayerState->OnRep_SquadId();
    }

    if (GameState->HasGameMemberInfoArray())
    {
        auto Member = (FGameMemberInfo*)malloc(FGameMemberInfo::Size());
        memset((PBYTE)Member, 0, FGameMemberInfo::Size());

        Member->MostRecentArrayReplicationKey = -1;
        Member->ReplicationID = -1;
        Member->ReplicationKey = -1;
        Member->TeamIndex = PlayerState->TeamIndex;
        Member->SquadId = PlayerState->SquadId;
        Member->MemberUniqueId = PlayerState->HasUniqueID() ? PlayerState->UniqueID : PlayerState->UniqueId;

        GameState->GameMemberInfoArray.Members.Add(*Member, FGameMemberInfo::Size());
        GameState->GameMemberInfoArray.MarkItemDirty(*Member);
        
        auto NotifyGameMemberAdded = (void(*)(AFortGameStateAthena*, uint8_t, uint8_t, FUniqueNetIdRepl*)) NotifyGameMemberAdded_;
        if (NotifyGameMemberAdded)
            NotifyGameMemberAdded(GameState, Member->SquadId, Member->TeamIndex, &Member->MemberUniqueId);

        free(Member);
    }

    if (NewPlayer->HasbBuildFree())
        NewPlayer->bBuildFree = FConfiguration::bInfiniteMats;

    if (!NewPlayer->WorldInventory)
    {
        NewPlayer->WorldInventory = UWorld::SpawnActor<AFortInventory>(NewPlayer->WorldInventoryClass, FVector{});
        NewPlayer->WorldInventory->SetOwner(NewPlayer);
    }

    if (wcsstr(FConfiguration::Playlist, L"/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2"))
    { }

    return callOG(GameMode, Stack.GetCurrentNativeFunction(), HandleStartingNewPlayer, NewPlayer);
}


uint8_t AFortGameModeAthena::PickTeam(AFortGameModeAthena* GameMode, uint8_t PreferredTeam, AFortPlayerControllerAthena* Controller)
{
    uint8_t ret = CurrentTeam;

    if (bIsLargeTeamGame)
    {
        if (CurrentTeam == 4)
            CurrentTeam = 3;
        else
            CurrentTeam = 4;
    }
    else
    {
        auto Playlist = VersionInfo.FortniteVersion >= 4.0 ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData) : nullptr;
        if (++PlayersOnCurTeam >= (Playlist ? Playlist->MaxSquadSize : 1))
        {
            CurrentTeam++;
            PlayersOnCurTeam = 0;
        }
    }

    return ret;
}

bool AFortGameModeAthena::StartAircraftPhase(AFortGameModeAthena* GameMode, char a2)
{
    auto Ret = StartAircraftPhaseOG(GameMode, a2);

    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    GUI::gsStatus = 2;
    sprintf_s(GUI::windowTitle, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Match started" : (VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "Erbium (FN %.2f, UE %.2f): Match started" : "Erbium (FN %.1f, UE %.2f): Match started"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
    SetConsoleTitleA(GUI::windowTitle);

    if (FConfiguration::bLateGame && VersionInfo.FortniteVersion < 25.20)
    {
        /*if (VersionInfo.FortniteVersion < 16)
        {
            GameState->GamePhase = 4;
            GameState->GamePhaseStep = 7;
            GameState->OnRep_GamePhase(3);
        }*/

        auto Aircraft = GameState->HasAircrafts() ? GameState->Aircrafts[0] : GameState->Aircraft;
        if (GameMode->SafeZoneLocations.Num() < 4)
        {
            FConfiguration::bLateGame = false;
            printf("LateGame is not supported on this version!\n");
            return Ret;
        }
        FVector Loc = GameMode->SafeZoneLocations.Get(FConfiguration::LateGameZone + (VersionInfo.FortniteVersion >= 24 ? 2 : 0) - 1, FVector::Size());
        Loc.Z = 17500.f;
        if (GameState->HasDefaultParachuteDeployTraceForGroundDistance())
            GameState->DefaultParachuteDeployTraceForGroundDistance = 2500.f;

        if (Aircraft->HasFlightInfo())
        {
            Aircraft->FlightInfo.FlightSpeed = 0.f;

            Aircraft->FlightInfo.FlightStartLocation = Loc;

            Aircraft->FlightInfo.TimeTillFlightEnd = 7.f;
            Aircraft->FlightInfo.TimeTillDropEnd = 0.f;
            Aircraft->FlightInfo.TimeTillDropStart = 0.f;
        }
        else
        {
            Aircraft->FlightSpeed = 0.f;

            Aircraft->FlightStartLocation = Loc;

            Aircraft->TimeTillFlightEnd = 7.f;
            Aircraft->TimeTillDropEnd = 0.f;
            Aircraft->TimeTillDropStart = 0.f;
        }
        Aircraft->FlightStartTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        Aircraft->FlightEndTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 7.f;
        //GameState->bAircraftIsLocked = false;
        //GameState->SafeZonesStartTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 8.f;
    }

    return Ret;
}


void AFortGameModeAthena::OnAircraftExitedDropZone_(UObject* Context, FFrame& Stack)
{
    AFortAthenaAircraft* Aircraft;
    Stack.StepCompiledIn(&Aircraft);
    Stack.IncrementCode();

    auto GameMode = (AFortGameModeAthena*)Context;
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    if (FConfiguration::bLateGame)
    {
        static auto CompClass = FindClass("FortControllerComponent_Aircraft");

        if (CompClass)
        {
            for (auto& Player : GameMode->AlivePlayers)
            {
                if (((AFortPlayerControllerAthena*)Player)->IsInAircraft())
                {
                    ((AFortPlayerControllerAthena*)Player)->GetAircraftComponent()->ServerAttemptAircraftJump(FRotator{});
                }
            }
        }
        else
        {
            for (auto& Player : GameMode->AlivePlayers)
            {
                if (((AFortPlayerControllerAthena*)Player)->IsInAircraft())
                {
                    ((AFortPlayerControllerAthena*)Player)->ServerAttemptAircraftJump(FRotator{});
                }
            }
        }
    }

    if (FConfiguration::bLateGame)
    {
        GameState->GamePhase = 4;
        GameState->GamePhaseStep = 7;
        GameState->OnRep_GamePhase(3);
    }

    callOG(GameMode, Stack.GetCurrentNativeFunction(), OnAircraftExitedDropZone, Aircraft);
}

TArray<FFortSafeZonePhaseInfo> Phases;

AFortSafeZoneIndicator* SetupSafeZoneIndicator(AFortGameModeAthena* GameMode)
{
    // thanks heliato
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    if (!GameMode->SafeZoneIndicator)
    {
        AFortSafeZoneIndicator* SafeZoneIndicator = UWorld::SpawnActor<AFortSafeZoneIndicator>(GameMode->SafeZoneIndicatorClass, FVector{});

        if (SafeZoneIndicator)
        {
            FFortSafeZoneDefinition& SafeZoneDefinition = GameState->MapInfo->SafeZoneDefinition;
            float SafeZoneCount = SafeZoneDefinition.Count.Evaluate();

            auto& Array = SafeZoneIndicator->HasSafeZonePhases() ? SafeZoneIndicator->SafeZonePhases : Phases;


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
                PhaseInfo->TimeBetweenStormCapDamage = GameMode->TimeBetweenStormCapDamage.Evaluate(i);
                PhaseInfo->StormCapDamagePerTick = GameMode->StormCapDamagePerTick.Evaluate(i);
                PhaseInfo->StormCampingIncrementTimeAfterDelay = GameMode->StormCampingIncrementTimeAfterDelay.Evaluate(i);
                PhaseInfo->StormCampingInitialDelayTime = GameMode->StormCampingInitialDelayTime.Evaluate(i);
                PhaseInfo->MegaStormGridCellThickness = (int)SafeZoneDefinition.MegaStormGridCellThickness.Evaluate(i);

                if (FFortSafeZonePhaseInfo::HasUsePOIStormCenter())
                    PhaseInfo->UsePOIStormCenter = false;

                if (GameMode->SafeZoneLocations.GetData() && GameMode->SafeZoneLocations.Num() > i)
                    PhaseInfo->Center = GameMode->SafeZoneLocations.Get((int)i, FVector::Size());

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

        GameMode->SafeZoneIndicator = SafeZoneIndicator;
        GameState->SafeZoneIndicator = SafeZoneIndicator;
        GameState->OnRep_SafeZoneIndicator();
    }

    return GameMode->SafeZoneIndicator;
}

void StartNewSafeZonePhase(AFortGameModeAthena* GameMode, int NewSafeZonePhase)
{
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    float TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(GameState);
    auto& Array = GameMode->SafeZoneIndicator->HasSafeZonePhases() ? GameMode->SafeZoneIndicator->SafeZonePhases : Phases;

    if (Array.IsValidIndex(NewSafeZonePhase))
    {
        if (Array.IsValidIndex(NewSafeZonePhase - 1))
        {
            auto& PreviousPhaseInfo = Array.Get(NewSafeZonePhase - 1, FFortSafeZonePhaseInfo::Size());

            GameMode->SafeZoneIndicator->PreviousCenter = PreviousPhaseInfo.Center;
            GameMode->SafeZoneIndicator->PreviousRadius = PreviousPhaseInfo.Radius;
        }

        auto& PhaseInfo = Array.Get(NewSafeZonePhase, FFortSafeZonePhaseInfo::Size());

        GameMode->SafeZoneIndicator->NextCenter = PhaseInfo.Center;
        GameMode->SafeZoneIndicator->NextRadius = PhaseInfo.Radius;
        GameMode->SafeZoneIndicator->NextMegaStormGridCellThickness = PhaseInfo.MegaStormGridCellThickness;

        if (Array.IsValidIndex(NewSafeZonePhase + 1))
        {
            auto& NextPhaseInfo = Array.Get(NewSafeZonePhase + 1, FFortSafeZonePhaseInfo::Size());

            GameMode->SafeZoneIndicator->FutureReplicator->NextNextCenter = NextPhaseInfo.Center;
            GameMode->SafeZoneIndicator->FutureReplicator->NextNextRadius = NextPhaseInfo.Radius;

            GameMode->SafeZoneIndicator->NextNextCenter = NextPhaseInfo.Center;
            GameMode->SafeZoneIndicator->NextNextRadius = NextPhaseInfo.Radius;
            GameMode->SafeZoneIndicator->NextNextMegaStormGridCellThickness = NextPhaseInfo.MegaStormGridCellThickness;
        }

        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = FConfiguration::bLateGame && FConfiguration::bLateGameLongZone ? 676767.f : TimeSeconds + PhaseInfo.WaitTime;
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + PhaseInfo.ShrinkTime;

        GameMode->SafeZoneIndicator->CurrentDamageInfo = PhaseInfo.DamageInfo;
        GameMode->SafeZoneIndicator->OnRep_CurrentDamageInfo();

        GameMode->SafeZoneIndicator->CurrentPhase = NewSafeZonePhase;
        GameMode->SafeZoneIndicator->OnRep_CurrentPhase();

        GameMode->SafeZoneIndicator->OnSafeZonePhaseChanged.Process();
    }
}

void (*SpawnInitialSafeZoneOG)(AFortGameModeAthena* GameMode);
void SpawnInitialSafeZone(AFortGameModeAthena* GameMode)
{
    //return;
    GameMode->bSafeZoneActive = true;
    auto SafeZoneIndicator = SetupSafeZoneIndicator(GameMode);

    SafeZoneIndicator->OnSafeZonePhaseChanged.Bind(GameMode, FName(L"HandlePostSafeZonePhaseChanged"));
    GameMode->OnSafeZoneIndicatorSpawned.Process(SafeZoneIndicator);

    StartNewSafeZonePhase(GameMode, FConfiguration::bLateGame ? (FConfiguration::LateGameZone + (VersionInfo.FortniteVersion >= 24 ? 2 : 0)) : 1);


    //return SpawnInitialSafeZoneOG(GameMode);
}

void (*UpdateSafeZonesPhaseOG)(AFortGameModeAthena* GameMode);
void UpdateSafeZonesPhase(AFortGameModeAthena* GameMode)
{
    if (GameMode->bSafeZoneActive && UGameplayStatics::GetTimeSeconds(GameMode) >= GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime && !GameMode->bSafeZonePaused)
        StartNewSafeZonePhase(GameMode, GameMode->SafeZoneIndicator->CurrentPhase + 1);

    return UpdateSafeZonesPhaseOG(GameMode);
}


void GetPhaseInfo(UObject* Context, FFrame& Stack, bool* Ret)
{
    auto& OutSafeZonePhase = Stack.StepCompiledInRef<FFortSafeZonePhaseInfo>();
    int32 InPhaseToGet;
    Stack.StepCompiledIn(&InPhaseToGet);
    Stack.IncrementCode();
    auto SafeZoneIndicator = (AFortSafeZoneIndicator*)Context;
    auto& Array = SafeZoneIndicator->HasSafeZonePhases() ? SafeZoneIndicator->SafeZonePhases : Phases;

    if (Array.IsValidIndex(InPhaseToGet))
    {
        OutSafeZonePhase = Array[InPhaseToGet];

        *Ret = true;
        return;
    }
    *Ret = false;
}

void AFortGameModeAthena::Hook()
{
    Utils::ExecHook(GetDefaultObj()->GetFunction("ReadyToStartMatch"), ReadyToStartMatch_, ReadyToStartMatch_OG);

    if (VersionInfo.EngineVersion >= 5.1)
    {
        auto sRef = Memcury::Scanner::FindStringRef(L"STAT_LoadMap").Get();

        uint64_t LoadMapAddr = 0;
        for (int i = 0; i < 3000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
            {
                LoadMapAddr = sRef - i;
                break;
            }

            if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
            {
                LoadMapAddr = sRef - i;
                break;
            }
        }
    }
}

void AFortGameModeAthena::PostLoadHook()
{
    ApplyCharacterCustomization = FindApplyCharacterCustomization();
    NotifyGameMemberAdded_ = FindNotifyGameMemberAdded();

    auto spdf = GetDefaultObj()->GetFunction("SpawnDefaultPawnFor");
    SpawnDefaultPawnForIdx = spdf->GetVTableIndex();

    Utils::ExecHook(spdf, SpawnDefaultPawnFor);
    Utils::ExecHook(GetDefaultObj()->GetFunction("HandleStartingNewPlayer"), HandleStartingNewPlayer_, HandleStartingNewPlayer_OG);
    Utils::Hook(FindPickTeam(), PickTeam, PickTeamOG);
    if (VersionInfo.FortniteVersion < 25.20)
    {
        Utils::Hook(FindStartAircraftPhase(), StartAircraftPhase, StartAircraftPhaseOG);
        Utils::Hook(FindHandlePostSafeZonePhaseChanged(), HandlePostSafeZonePhaseChanged, HandlePostSafeZonePhaseChangedOG);
    }
    Utils::ExecHook(GetDefaultObj()->GetFunction("OnAircraftExitedDropZone"), OnAircraftExitedDropZone_, OnAircraftExitedDropZone_OG);

    if (VersionInfo.FortniteVersion >= 21.50)
    {
        if (VersionInfo.FortniteVersion < 25.20)
        {
            Utils::Hook(FindSpawnInitialSafeZone(), SpawnInitialSafeZone, SpawnInitialSafeZoneOG);
            Utils::Hook(FindUpdateSafeZonesPhase(), UpdateSafeZonesPhase, UpdateSafeZonesPhaseOG);
        }
        Utils::ExecHook(L"/Script/FortniteGame.FortSafeZoneIndicator.GetPhaseInfo", GetPhaseInfo);
    }
}
