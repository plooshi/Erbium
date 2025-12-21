#include "pch.h"
#include "../Public/FortGameMode.h"
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
#include "../Public/FortAthenaCreativePortal.h"
#include "../Public/FortPhysicsPawn.h"

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

void SetupPlaylist(AFortGameMode* GameMode, AFortGameStateAthena* GameState)
{
    auto Playlist = FindObject<UFortPlaylistAthena>(FConfiguration::Playlist);

    if (!Playlist)
        Playlist = FindObject<UFortPlaylistAthena>(L"/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");

    if (Playlist)
    {
        if (FConfiguration::bForceRespawns)
        {
            if (Playlist->HasbRespawnInAir())
                Playlist->bRespawnInAir = true;
            if (Playlist->HasRespawnHeight())
            {
                Playlist->RespawnHeight.Curve.CurveTable = nullptr;
                Playlist->RespawnHeight.Curve.RowName = FName();
                Playlist->RespawnHeight.Value = 20000;
            }
            if (Playlist->HasRespawnTime())
            {
                Playlist->RespawnTime.Curve.CurveTable = nullptr;
                Playlist->RespawnTime.Curve.RowName = FName();
                Playlist->RespawnTime.Value = 3;
            }
            Playlist->RespawnType = 1; // InfiniteRespawns
            if (Playlist->HasbForceRespawnLocationInsideOfVolume())
                Playlist->bForceRespawnLocationInsideOfVolume = true;
        }
        if (FConfiguration::bJoinInProgress)
        {
            if (Playlist->HasbAllowJoinInProgress())
                Playlist->bAllowJoinInProgress = true;
            if (Playlist->HasJoinInProgressMatchType())
                Playlist->JoinInProgressMatchType = UKismetTextLibrary::Conv_StringToText(FString(L"Creative"));
        }
        //if (VersionInfo.FortniteVersion >= 16)
        {
            if (Playlist->HasGarbageCollectionFrequency())
                Playlist->GarbageCollectionFrequency = 9999999999999999.f; // easier than hooking collectgarbage
            if (GameMode->HasPlaylistHotfixOriginalGCFrequency())
                GameMode->PlaylistHotfixOriginalGCFrequency = 9999999999999999.f;
            if (GameMode->HasbDisableGCOnServerDuringMatch())
                GameMode->bDisableGCOnServerDuringMatch = true;
            if (GameMode->HasbPlaylistHotfixChangedGCDisabling())
                GameMode->bPlaylistHotfixChangedGCDisabling = true;
        }
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

        //if (GameState->HasAdditionalPlaylistLevelsStreamed())
        //    GameState->OnRep_AdditionalPlaylistLevelsStreamed();
    }
    else
    {
        GameState->CurrentPlaylistId = GameMode->CurrentPlaylistId = 0;

        if (GameMode->GameSession->HasMaxPlayers())
            GameMode->GameSession->MaxPlayers = 100;
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

class AFortAthenaLivingWorldStaticPointProvider : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaLivingWorldStaticPointProvider);

    DEFINE_PROP(FiltersTags, FGameplayTagContainer);
};

class AFortAthenaVehicleSpawner : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaVehicleSpawner);

    DEFINE_FUNC(GetVehicleClass, UClass*);
};

void AFortGameMode::ReadyToStartMatch_(UObject* Context, FFrame& Stack, bool* Ret)
{
    Stack.IncrementCode();

    static auto FrontendMode = FindClass("FortGameModeFrontend");
    if (Context->IsA(FrontendMode))
    {
        *Ret = callOGWithRet(((AFortGameMode*)Context), Stack.GetCurrentNativeFunction(), ReadyToStartMatch);
        return;
    }
    auto GameMode = Context->Cast<AFortGameMode>();

    auto GameState = GameMode->GameState;

    static bool setup = false;
    if (GameMode->HasWarmupRequiredPlayerCount() ? GameMode->WarmupRequiredPlayerCount != 1 : !setup)
    {
        setup = true;

        //if (!FindListenCall())
        {
            auto World = UWorld::GetWorld();
            auto Engine = UEngine::GetEngine();
            auto NetDriverName = FName(L"GameNetDriver");

            if (GameMode->HasbEnableReplicationGraph())
                GameMode->bEnableReplicationGraph = true;

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
            URL->Port = FConfiguration::Port;

            auto InitListen = (bool (*)(UNetDriver*, UWorld*, FURL*, bool, FString&)) FindInitListen();
            auto SetWorld = (void (*)(UNetDriver*, UWorld*)) FindSetWorld();

            SetWorld(NetDriver, World);
            FString Err;
            if (InitListen(NetDriver, World, URL, false, Err))
                SetWorld(NetDriver, World);
            else
                printf("Failed to listen!");

            free(URL);
        }

        if (GameMode->HasWarmupRequiredPlayerCount())
            GameMode->WarmupRequiredPlayerCount = 1;

        if (VersionInfo.FortniteVersion >= 4.0 && (VersionInfo.EngineVersion < 4.22 || VersionInfo.EngineVersion >= 4.26))
            SetupPlaylist(GameMode, GameState);


        auto Playlist = FindObject<UFortPlaylistAthena>(FConfiguration::Playlist);

        if (!Playlist)
            Playlist = FindObject<UFortPlaylistAthena>(L"/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo");

        if (Playlist)
        {
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
        }

        // misc C1 poi things
        if (VersionInfo.FortniteVersion >= 6 && VersionInfo.FortniteVersion < 7)
        {
            if (VersionInfo.FortniteVersion > 6.10)
                ShowFoundation(VersionInfo.FortniteVersion <= 6.21 ? FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Lake1") : FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Lake2"));
            else
                ShowFoundation(FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_StreamingTest12"));

            ShowFoundation(VersionInfo.FortniteVersion <= 6.10 ? FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_StreamingTest13") : FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_FloatingIsland"));

            auto IslandScripting = TUObjectArray::FindFirstObject("BP_IslandScripting_C");
            if (IslandScripting)
            {
                auto UpdateMapOffset = IslandScripting->GetOffset("UpdateMap");
                if (UpdateMapOffset != -1)
                {
                    *(bool*)(__int64(IslandScripting) + UpdateMapOffset) = true;
                    IslandScripting->ProcessEvent(IslandScripting->GetFunction("OnRep_UpdateMap"), nullptr);
                }
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
                        ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.PleasentParkFestivus"));

                    break;
                }
            }
        }

        if (VersionInfo.FortniteVersion == 12.41)
        {
            ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.LF_Athena_POI_19x19_2"));
            ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head6_18"));
            ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head5_14"));
            ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head3_8"));
            ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head_2"));
            ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.BP_Jerky_Head4_11"));
        }

        if (VersionInfo.FortniteVersion == 7.30 && !bEvent)
            ShowFoundation(FindObject<ABuildingFoundation>("/Game/Athena/Apollo/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.PleasentParkDefault"));


        if (VersionInfo.EngineVersion >= 4.27 && std::floor(VersionInfo.FortniteVersion) != 20) // on 20 it does some weird stuff
        {
            auto MeshNetworkSubsystem = TUObjectArray::FindFirstObject("MeshNetworkSubsystem");

            if (MeshNetworkSubsystem)
                *(uint8_t*)(__int64(MeshNetworkSubsystem) + MeshNetworkSubsystem->GetOffset("NodeType")) = 2;
        }

        auto AIDirectorClass = GameMode->HasWarmupRequiredPlayerCount() ? FindClass("AthenaAIDirector") : FindObject<UClass>("/Game/AIDirector/AIDirector_Fortnite.AIDirector_Fortnite_C");
        if (!AIDirectorClass)
            AIDirectorClass = FindClass("FortAIDirector");

        GameMode->AIDirector = UWorld::SpawnActor(AIDirectorClass, FVector{}, GameMode);
        if (GameMode->AIDirector)
            GameMode->AIDirector->Call(GameMode->AIDirector->GetFunction("Activate"));

        if (UFortServerBotManagerAthena::StaticClass())
        {
            if (auto BotManager = (UFortServerBotManagerAthena*)UGameplayStatics::SpawnObject(UFortServerBotManagerAthena::StaticClass(), GameMode))
            {
                GameMode->ServerBotManager = BotManager;
                BotManager->CachedGameState = GameState;
                BotManager->CachedGameMode = GameMode;
            }
            else
            {
                printf("BotManager is nullptr!\n");
            }
        }

        auto GoalManagerClass = GameMode->HasWarmupRequiredPlayerCount() ? FindClass("FortAIGoalManager") : FindObject<UClass>("/Game/AI/GoalSelection/AIGoalManager.AIGoalManager_C");

        GameMode->AIGoalManager = UWorld::SpawnActor(GoalManagerClass, FVector{}, GameMode);

        auto MissionManagerClass = GameMode->HasWarmupRequiredPlayerCount() ? nullptr : FindObject<UClass>("/Game/Blueprints/MissionManager.MissionManager_C");

        if (MissionManagerClass)
        {
            GameState->MissionManager = UWorld::SpawnActor(MissionManagerClass, FVector{}, GameState);
            GameState->OnRep_MissionManager();

            auto MissionInfo = FindObject<UFortMissionInfo>(L"/Game/Missions/Primary/EvacuateTheSurvivors/EvacuteTheSurvivors.EvacuteTheSurvivors");

            if (!MissionInfo)
                MissionInfo = FindObject<UFortMissionInfo>(L"/SaveTheWorld/Missions/Primary/EvacuateTheSurvivors/EvacuteTheSurvivors.EvacuteTheSurvivors");

            if (MissionInfo)
            {
                MissionInfo->bStartPlayingOnLoad = true; // bad hack, we should find a better way to do this later
                // startplayingmission

                UFortMissionLibrary::LoadMission(UWorld::GetWorld(), MissionInfo);
            }
            // we need to spawn bluglo manager too?
        }

        /* if (VersionInfo.EngineVersion == 4.16)
         {
             if (!UWorld::GetWorld()->NavigationSystem)
             {
                 UWorld::GetWorld()->NavigationSystem = UGameplayStatics::SpawnObject(FindClass("FortNavSystem"), UWorld::GetWorld());
                 auto OnWorldInitDone = (void(*)(UObject*, char))(ImageBase + 0x1f6fc40);
                 OnWorldInitDone(UWorld::GetWorld()->NavigationSystem, 1);
             }
         }*/

         //if (!GameMode->HasWarmupRequiredPlayerCount())
         //    UWorld::SpawnActor(FindClass("FortPlayerStart"), FVector{0, 0, 3000});

        *Ret = false;
        return;
    }

    if (!GameMode->bWorldIsReady)
    {
        static auto WarmupStartClass = FindClass("PlayerStart");
        TArray<AActor*> Starts;
        Utils::GetAll(WarmupStartClass, Starts);
        auto StartsNum = Starts.Num();
        Starts.Free();

        if (StartsNum == 0 || !Misc::bHookedAll)
        {
            *Ret = false;
            return;
        }

        TArray<AFortAthenaMapInfo*> AllMapInfos;
        Utils::GetAll<AFortAthenaMapInfo>(AllMapInfos);

        if (AllMapInfos.Num() > 0 && !GameState->MapInfo)
        {
            *Ret = false;
            return;
        }
        AllMapInfos.Free();



        if ((VersionInfo.FortniteVersion >= 3.5 && VersionInfo.FortniteVersion <= 4.0) || (VersionInfo.EngineVersion >= 4.22 && VersionInfo.EngineVersion < 4.26))
            SetupPlaylist(GameMode, GameState);
        //else if (VersionInfo.EngineVersion >= 4.22 && VersionInfo.EngineVersion < 4.26)
        //    GameState->OnRep_CurrentPlaylistInfo();

        if (VersionInfo.FortniteVersion >= 25.20 && GameState->HasMapInfo() && GameState->MapInfo)
        {
            auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameState);

            if (GamePhaseLogic)
            {
                auto InitializeFlightPath = (void(*)(AFortAthenaMapInfo*, AFortGameStateAthena*, UFortGameStateComponent_BattleRoyaleGamePhaseLogic*, bool, double, float, float)) FindInitializeFlightPath();
                if (InitializeFlightPath)
                    InitializeFlightPath(GameState->MapInfo, GameState, GamePhaseLogic, false, 0.f, 0.f, 360.f);
                UFortGameStateComponent_BattleRoyaleGamePhaseLogic::GenerateStormCircles(GameState->MapInfo);
            }
        }

        auto Playlist = VersionInfo.FortniteVersion >= 3.5 && GameMode->HasWarmupRequiredPlayerCount() ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData) : nullptr;

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
                                TArray<AActor*> AllLoaders;
                                Utils::GetAll(LoaderClass, AllLoaders);
                                LoaderObject = AllLoaders.Num() > 0 ? AllLoaders[0] : nullptr;
                                AllLoaders.Free();
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

        auto AbilitySet = VersionInfo.FortniteVersion > 8.30 ? FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer") : FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_DefaultPlayer.GAS_DefaultPlayer");
        AbilitySet->AddToRoot();
        AbilitySets.Add(AbilitySet);

        if (VersionInfo.FortniteVersion >= 20)
        {
            auto TacticalSprintAbility = FindObject<UFortAbilitySet>(L"/TacticalSprintGame/Gameplay/AS_TacticalSprint.AS_TacticalSprint");

            if (!TacticalSprintAbility)
                TacticalSprintAbility = FindObject<UFortAbilitySet>(L"/TacticalSprint/Gameplay/AS_TacticalSprint.AS_TacticalSprint");
            TacticalSprintAbility->AddToRoot();
            AbilitySets.Add(TacticalSprintAbility);

            auto AscenderAbility = FindObject<UFortAbilitySet>(L"/Ascender/Gameplay/Ascender/AS_Ascender.AS_Ascender");
            AscenderAbility->AddToRoot();
            AbilitySets.Add(AscenderAbility);

            auto DoorBashAbility = FindObject<UFortAbilitySet>(L"/DoorBashContent/Gameplay/AS_DoorBash.AS_DoorBash");
            DoorBashAbility->AddToRoot();
            AbilitySets.Add(DoorBashAbility);

            auto HillScrambleAbility = FindObject<UFortAbilitySet>(L"/HillScramble/Gameplay/AS_HillScramble.AS_HillScramble");
            HillScrambleAbility->AddToRoot();
            AbilitySets.Add(HillScrambleAbility);

            auto SlideImpulseAbility = FindObject<UFortAbilitySet>(L"/SlideImpulse/Gameplay/AS_SlideImpulse.AS_SlideImpulse");
            SlideImpulseAbility->AddToRoot();
            AbilitySets.Add(SlideImpulseAbility);

            if (std::floor(VersionInfo.FortniteVersion) == 21)
            {
                auto RealitySaplingAbility = FindObject<UFortAbilitySet>(L"/RealitySeedGameplay/Environment/Foliage/GAS_Athena_RealitySapling.GAS_Athena_RealitySapling");
                AbilitySets.Add(RealitySaplingAbility);
            }
        }

        for (auto& Set : AbilitySets)
            if (Set)
                Set->AddToRoot();

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

        auto AddToTierData = [&](const UDataTable* Table, TArray<FFortLootTierData*>& TempArr)
            {
                if (!Table)
                    return;

                Table->AddToRoot();
                if (VersionInfo.FortniteVersion >= 20)
                {
                    if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
                        for (auto& ParentTable : CompositeTable->ParentTables)
                            if (ParentTable)
                                for (auto& [Key, Val] : *(TMap<int32, FFortLootTierData*>*) (__int64(ParentTable) + 0x30))
                                    TempArr.Add(Val);

                    for (auto& [Key, Val] : *(TMap<int32, FFortLootTierData*>*) (__int64(Table) + 0x30))
                    {
                        bool bFound = false;

                        for (auto& TierData : TempArr)
                            if (TierData->TierGroup == Val->TierGroup && TierData->LootPackage == Val->LootPackage)
                            {
                                TierData = Val;
                                bFound = true;
                                break;
                            }

                        if (!bFound)
                            TempArr.Add(Val);
                    }
                }
                else
                {
                    if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
                        for (auto& ParentTable : CompositeTable->ParentTables)
                            if (ParentTable)
                                for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) ParentTable->RowMap)
                                    TempArr.Add(Val);

                    for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) Table->RowMap)
                    {
                        bool bFound = false;

                        for (auto& TierData : TempArr)
                            if (TierData->TierGroup == Val->TierGroup && TierData->LootPackage == Val->LootPackage)
                            {
                                TierData = Val;
                                bFound = true;
                                break;
                            }

                        if (!bFound)
                            TempArr.Add(Val);
                    }
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
                            if (ParentTable)
                                for (auto& [Key, Val] : *(TMap<int32, FFortLootPackageData*>*) (__int64(ParentTable) + 0x30))
                                    TempArr[Key] = Val;

                    for (auto& [Key, Val] : *(TMap<int32, FFortLootPackageData*>*) (__int64(Table) + 0x30))
                        TempArr[Key] = Val;
                }
                else
                {
                    if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
                        for (auto& ParentTable : CompositeTable->ParentTables)
                            if (ParentTable)
                                for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>) ParentTable->RowMap)
                                    TempArr[Key.ComparisonIndex] = Val;

                    for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>) Table->RowMap)
                    {
                        TempArr[Key.ComparisonIndex] = Val;
                    }
                }
            };

        TArray<FFortLootTierData*> LootTierDataTempArr;
        auto LootTierData = Playlist ? Playlist->LootTierData.Get() : nullptr;
        if (!LootTierData)
            LootTierData = FindObject<UDataTable>(GameMode->HasWarmupRequiredPlayerCount() ? L"/Game/Items/Datatables/AthenaLootTierData_Client.AthenaLootTierData_Client" : L"/Game/Items/Datatables/LootTierData_Client.LootTierData_Client");
        if (LootTierData)
            AddToTierData(LootTierData, LootTierDataTempArr);

        for (auto& Val : LootTierDataTempArr)
            TierDataMap[Val->TierGroup.ComparisonIndex].Add(Val);

        UEAllocatedMap<int32, FFortLootPackageData*> LootPackageTempArr;
        auto LootPackages = Playlist ? Playlist->LootPackages.Get() : nullptr;
        if (!LootPackages)
            LootPackages = FindObject<UDataTable>(GameMode->HasWarmupRequiredPlayerCount() ? L"/Game/Items/Datatables/AthenaLootPackages_Client.AthenaLootPackages_Client" : L"/Game/Items/Datatables/LootPackages_Client.LootPackages_Client");
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
                        TArray<FFortLootTierData*> LTDTempData;

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

                        for (auto& Val : LTDTempData)
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

        /*if (floor(VersionInfo.FortniteVersion) != 20)
        {
            UFortLootPackage::SpawnFloorLootForContainer(FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C"));
            UFortLootPackage::SpawnFloorLootForContainer(FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C"));
        }

        auto ConsumableSpawners = Utils::GetAll<ABGAConsumableSpawner>();

        for (auto& Spawner : ConsumableSpawners)
            UFortLootPackage::SpawnConsumableActor(Spawner);*/

        if (VersionInfo.EngineVersion >= 4.27)
        {
            if (GameState->HasDefaultParachuteDeployTraceForGroundDistance())
                GameState->DefaultParachuteDeployTraceForGroundDistance = 10000;
        }

        if (VersionInfo.FortniteVersion >= 27)
        {
            // fix grind rails
            auto GameData = FindObject<UCurveTable>("/GrindRail/DataTables/GrindRailGameData.GrindRailGameData");

            if (GameData)
            {
                static FName UseGrindingMME = FName(L"Default.GrindRails.UseGrindingMME");

                for (const auto& [RowName, RowPtr] : GameData->RowMap)
                {
                    if (RowName != UseGrindingMME)
                        continue;

                    FSimpleCurve* Row = (FSimpleCurve*)RowPtr;

                    if (!Row)
                        continue;

                    for (int i = 0; i < Row->Keys.Num(); i++)
                    {
                        auto& Key = Row->Keys.Get(i, FSimpleCurveKey::Size());

                        Key.Value = 0.f;
                    }
                }
            }
        }

        const UObject* BattleBusDef = nullptr;
        const UClass* SupplyDropClass = nullptr;
        if (VersionInfo.FortniteVersion == 18.40)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_HeadbandBus.BBID_HeadbandBus");
        else if (VersionInfo.FortniteVersion == 1.11 || VersionInfo.FortniteVersion == 7.30 || VersionInfo.FortniteVersion == 11.31 || VersionInfo.FortniteVersion == 15.10 || VersionInfo.FortniteVersion == 19.01 || VersionInfo.FortniteVersion == 28.01)
        {
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_WinterBus.BBID_WinterBus");

            if (VersionInfo.FortniteVersion == 1.11)
                SupplyDropClass = FindObject<UClass>(L"/Game/Athena/SupplyDrops/B_AthenaSupplyDrop_Gift.B_AthenaSupplyDrop_Gift_C");
            else
                SupplyDropClass = FindObject<UClass>(L"/Game/Athena/SupplyDrops/AthenaSupplyDrop_Holiday.AthenaSupplyDrop_Holiday_C");
        }
        else if (VersionInfo.FortniteVersion == 23.10)
        {
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BattleBus_Booster_Winter.BBID_BattleBus_Booster_Winter");
            SupplyDropClass = FindObject<UClass>(L"/Game/Athena/SupplyDrops/AthenaSupplyDrop_Holiday.AthenaSupplyDrop_Holiday_C");
        }
        else if (VersionInfo.FortniteVersion == 5.10 || VersionInfo.FortniteVersion == 9.41 || VersionInfo.FortniteVersion == 14.20 || VersionInfo.FortniteVersion == 18.00 || VersionInfo.FortniteVersion == 22.00 || VersionInfo.FortniteVersion == 26.20)
        {
            if (VersionInfo.FortniteVersion == 5.10)
                BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus.BBID_BirthdayBus");
            else if (VersionInfo.FortniteVersion == 9.41)
                BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus2nd.BBID_BirthdayBus2nd");
            else if (VersionInfo.FortniteVersion == 14.20)
                BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus3rd.BBID_BirthdayBus3rd");
            else if (VersionInfo.FortniteVersion == 18.00)
                BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus4th.BBID_BirthdayBus4th");
            else if (VersionInfo.FortniteVersion == 22.00)
                BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus5th.BBID_BirthdayBus5th");
            else if (VersionInfo.FortniteVersion == 26.20)
                BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus6th.BBID_BirthdayBus6th");

            SupplyDropClass = FindObject<UClass>(L"/Game/Athena/SupplyDrops/AthenaSupplyDrop_BDay.AthenaSupplyDrop_BDay_C");
        }
        else if (VersionInfo.FortniteVersion == 6.20 || VersionInfo.FortniteVersion == 6.21 || VersionInfo.FortniteVersion == 11.10 || VersionInfo.FortniteVersion == 14.40 || VersionInfo.FortniteVersion == 18.21)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_HalloweenBus.BBID_HalloweenBus");
        else if (VersionInfo.FortniteVersion == 26.30)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_HalloweenBus_Booster.BBID_HalloweenBus_Booster");
        else if (VersionInfo.FortniteVersion == 14.30)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BusUpgrade1.BBID_BusUpgrade1");
        else if (VersionInfo.FortniteVersion == 14.50)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BusUpgrade2.BBID_BusUpgrade2");
        else if (VersionInfo.FortniteVersion == 14.60)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BusUpgrade3.BBID_BusUpgrade3");
        else if (VersionInfo.FortniteVersion >= 12.30 && VersionInfo.FortniteVersion <= 12.61)
        {
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_DonutBus.BBID_DonutBus");
            SupplyDropClass = FindObject<UClass>(L"/Game/Athena/SupplyDrops/AthenaSupplyDrop_Donut.AthenaSupplyDrop_Donut_C");
        }
        else if (VersionInfo.FortniteVersion == 9.30)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_WorldCupBus.BBID_WorldCupBus");
        else if (VersionInfo.FortniteVersion == 21.00)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_CelebrationBus.BBID_CelebrationBus");
        else if (std::floor(VersionInfo.FortniteVersion) == 27)
            BattleBusDef = FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_DefaultBus.BBID_DefaultBus");

        if (BattleBusDef)
        {
            if (GameState->HasDefaultBattleBus())
                GameState->DefaultBattleBus = BattleBusDef;

            TArray<AFortAthenaAircraft*> Aircrafts;
            Utils::GetAll<AFortAthenaAircraft>(Aircrafts);
            for (auto& Aircraft : Aircrafts)
            {
                Aircraft->DefaultBusSkin = BattleBusDef;

                if (Aircraft->SpawnedCosmeticActor)
                {
                    static auto Offset = Aircraft->SpawnedCosmeticActor->GetOffset("ActiveSkin");

                    GetFromOffset<const UObject*>(Aircraft->SpawnedCosmeticActor, Offset) = BattleBusDef;
                }
            }
            Aircrafts.Free();
        }

        if (GameState->HasMapInfo() && GameState->MapInfo)
        {
            if (SupplyDropClass)
            {
                if (GameState->MapInfo->HasSupplyDropInfoList())
                    for (auto& Info : GameState->MapInfo->SupplyDropInfoList)
                        Info->SupplyDropClass = SupplyDropClass;
                else
                    GameState->MapInfo->SupplyDropClass = SupplyDropClass;
            }

            if (VersionInfo.FortniteVersion >= 3.4)
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

            if (VersionInfo.FortniteVersion >= 3.3 && VersionInfo.FortniteVersion < 17 && GameState->MapInfo->LlamaClass)
            {
                auto PickSupplyDropLocation = (FVector * (*)(AFortAthenaMapInfo*, FVector*, FVector*, float, bool, float, float)) FindPickSupplyDropLocation();

                if (PickSupplyDropLocation)
                {
                    FFortSafeZoneDefinition& SafeZoneDefinition = GameState->MapInfo->SafeZoneDefinition;

                    auto LlamaMin = GameState->MapInfo->LlamaQuantityMin.Evaluate();
                    auto LlamaMax = GameState->MapInfo->LlamaQuantityMax.Evaluate();
                    auto LlamaCount = UKismetMathLibrary::RandomIntegerInRange((int)LlamaMin, (int)LlamaMax);
                    auto Radius = GameState->MapInfo->HasSafeZoneDefinition() ? SafeZoneDefinition.Radius.Evaluate(0) : 0;

                    if (Radius == 0)
                        Radius = 120000;
                    auto Center = GameState->MapInfo->GetMapCenter();
                    Center.Z = 10000;

                    for (int i = 0; i < LlamaCount; i++)
                    {
                        FVector Loc(0, 0, 0);
                        PickSupplyDropLocation(GameState->MapInfo, &Loc, &Center, Radius, 0, -1, -1);

                        if (Loc.X != 0 || Loc.Y != 0 || Loc.Z != 0)
                        {
                            FRotator Rot{};
                            Rot.Yaw = (float)rand() * 0.010986663f;

                            auto NewLlama = UWorld::SpawnActorUnfinished(GameState->MapInfo->LlamaClass, Loc, Rot);

                            static auto FindGroundLocationAt = NewLlama->GetFunction("FindGroundLocationAt");
                            auto GroundLoc = NewLlama->Call<FVector>(FindGroundLocationAt, Loc);

                            UWorld::FinishSpawnActor(NewLlama, GroundLoc, Rot);
                        }
                    }
                }
            }
        }

        GameMode->DefaultPawnClass = FindObject<UClass>(L"/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C");

        if (VersionInfo.EngineVersion == 4.16 && VersionInfo.FortniteVersion < 1.9)
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
        else if (VersionInfo.EngineVersion <= 4.20)
        {
            auto pattern = VersionInfo.FortniteVersion > 3.2 ? Memcury::Scanner::FindPattern("E8 ? ? ? ? EB 26 40 38 3D ? ? ? ?") : Memcury::Scanner::FindPattern("E8 ? ? ? ? F0 FF 0D ? ? ? ? 0F B6 C3");

            if (pattern.IsValid())
                Utils::Patch<uint8_t>(pattern.RelativeOffset(1).Get(), 0xC3);
        }

        if (GameState->HasAllPlayerBuildableClassesIndexLookup())
            for (auto& [Class, Handle] : GameState->AllPlayerBuildableClassesIndexLookup)
                AFortGameStateAthena::BuildingClassMap[Handle] = Class;

        if constexpr (FConfiguration::WebhookURL && *FConfiguration::WebhookURL)
        {
            auto curl = curl_easy_init();

            curl_easy_setopt(curl, CURLOPT_URL, FConfiguration::WebhookURL);
            curl_slist* headers = curl_slist_append(NULL, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            char version[6];

            sprintf_s(version, VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "%.2f" : "%.1f", VersionInfo.FortniteVersion);

            auto payload = UEAllocatedString("{\"embeds\": [{\"title\": \"Server is joinable!\", \"fields\": [{\"name\":\"Version\",\"value\":\"") + version + "\"}, {\"name\":\"Playlist\",\"value\":\"" + (Playlist ? Playlist->PlaylistName.ToString() : "Playlist_DefaultSolo") + "\"}], \"color\": " + "\"7237230\", \"footer\": {\"text\":\"Erbium\", \"icon_url\":\"https://cdn.discordapp.com/attachments/1341168629378584698/1436803905119064105/L0WnFa.png.png?ex=6910ef69&is=690f9de9&hm=01a0888b46647959b38ee58df322048ab49e2a5a678e52d4502d9c5e3978d805&\"}, \"timestamp\":\"" + iso8601() + "\"}] }";

            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

            curl_easy_perform(curl);

            curl_easy_cleanup(curl);
        }

        // for some reason it doesnt like when u do it earlier
        if (!Playlist && VersionInfo.FortniteVersion <= 4)
            if (GameMode->GameSession->HasMaxPlayers())
                GameMode->GameSession->MaxPlayers = 100;

        GUI::gsStatus = Joinable;
        sprintf_s(GUI::windowTitle, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Joinable" : (VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "Erbium (FN %.2f, UE %.2f): Joinable" : "Erbium (FN %.1f, UE %.2f): Joinable"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
        SetConsoleTitleA(GUI::windowTitle);
        GameMode->bWorldIsReady = true;
    }

    if (VersionInfo.EngineVersion >= 4.24 && GameMode->IsA<AFortGameModeAthena>())
    {
        int ReadyPlayers = 0;
        TArray<AFortPlayerControllerAthena*> PlayerList;
        Utils::GetAll<AFortPlayerControllerAthena>(PlayerList);

        for (auto& PlayerController : PlayerList)
        {
            auto PlayerState = PlayerController->PlayerState;

            if (!PlayerState->bIsSpectator && PlayerController->bReadyToStartMatch)
                ReadyPlayers++;
        }

        PlayerList.Free();

        auto VolumeManager = GameState->HasVolumeManager() ? GameState->VolumeManager : nullptr;

        bool bAllLevelsFinishedStreaming = true;
        if (GameState->HasAdditionalPlaylistLevelsStreamed())
        {
            TArray<FPlaylistStreamedLevelData>& AdditionalPlaylistLevels = *(TArray<FPlaylistStreamedLevelData>*) (__int64(GameState) + GameState->GetOffset("AdditionalPlaylistLevelsStreamed") - 0x10);
            for (int i = 0; i < AdditionalPlaylistLevels.Num(); i++)
            {
                auto& AdditionalPlaylistLevel = AdditionalPlaylistLevels.Get(i, FPlaylistStreamedLevelData::Size());

                if (!AdditionalPlaylistLevel.bIsFinishedStreaming || !AdditionalPlaylistLevel.StreamingLevel || !AdditionalPlaylistLevel.StreamingLevel->LoadedLevel->bIsVisible)
                {
                    bAllLevelsFinishedStreaming = false;
                    break;
                }
            }
        }

        static auto WaitingToStart = FName(L"WaitingToStart");
        *Ret = GameMode->bWorldIsReady && (GameState->HasbPlaylistDataIsLoaded() ? GameState->bPlaylistDataIsLoaded : true) && GameMode->MatchState == WaitingToStart && bAllLevelsFinishedStreaming && (!VolumeManager || !(VolumeManager->HasbInSpawningStartup() ? VolumeManager->bInSpawningStartup : GameState->bInSpawningStartup)) && ReadyPlayers >= (GameMode->HasWarmupRequiredPlayerCount() ? GameMode->WarmupRequiredPlayerCount : 1);
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

void AFortGameMode::SpawnDefaultPawnFor(UObject* Context, FFrame& Stack, AActor** Ret)
{
    AFortPlayerControllerAthena* NewPlayer;
    AActor* StartSpot;
    Stack.StepCompiledIn(&NewPlayer);
    Stack.StepCompiledIn(&StartSpot);
    Stack.IncrementCode();
    auto GameMode = (AFortGameMode*)Context;

    if (!NewPlayer || !StartSpot)
        return;

    auto GameState = GameMode->GameState;
    auto Num = NewPlayer->WorldInventory->Inventory.ReplicatedEntries.Num();
    AFortPlayerPawnAthena* Pawn = nullptr;

    Pawn = (AFortPlayerPawnAthena*)UWorld::SpawnActor(GameMode->GetDefaultPawnClassForController(NewPlayer), StartSpot->GetTransform(), NewPlayer, 3);

    while (!Pawn)
    {
        auto PlayerStart = GameMode->ChoosePlayerStart();
        if (PlayerStart)
            Pawn = (AFortPlayerPawnAthena*)UWorld::SpawnActor(GameMode->GetDefaultPawnClassForController(NewPlayer), PlayerStart->GetTransform(), NewPlayer, 3);
    }
    // they only stripped it on athena for some reason
    /*static auto FortGMSpawnDefaultPawnFor = (AFortPlayerPawnAthena * (*)(AFortGameMode*, AFortPlayerControllerAthena*, AActor*)) DefaultObjImpl("FortGameMode")->Vft[SpawnDefaultPawnForIdx];
    Pawn = FortGMSpawnDefaultPawnFor(GameMode, NewPlayer, StartSpot);

    if (!Pawn)
    {
        auto Transform = StartSpot->GetTransform();
        Transform.Translation.Z += 200.f;
        Pawn = GameMode->SpawnDefaultPawnAtTransform(NewPlayer, Transform);
    }*/


    if (Num == 0)
    {
        if (VersionInfo.FortniteVersion <= 1.91 && VersionInfo.FortniteVersion != 1.1 && VersionInfo.FortniteVersion != 1.11 && NewPlayer->HasStrongMyHero())
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

        if (NewPlayer->HasXPComponent())
        {
            if (NewPlayer->XPComponent->HasbRegisteredWithQuestManager())
            {
                NewPlayer->XPComponent->bRegisteredWithQuestManager = true;
                NewPlayer->XPComponent->OnRep_bRegisteredWithQuestManager();
            }

            if (NewPlayer->PlayerState->HasSeasonLevelUIDisplay())
            {
                NewPlayer->PlayerState->SeasonLevelUIDisplay = NewPlayer->XPComponent->CurrentLevel;
                NewPlayer->PlayerState->OnRep_SeasonLevelUIDisplay();
            }
            //NewPlayer->XPComponent->OnProfileUpdated();
        }

        static bool bFinalSetup = false;
        if (!bFinalSetup)
        {
            bFinalSetup = true;

            UFortLootPackage::SpawnFloorLootForContainer(FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C"));
            UFortLootPackage::SpawnFloorLootForContainer(FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C"));

            TArray<ABGAConsumableSpawner*> ConsumableSpawners{};
            Utils::GetAll<ABGAConsumableSpawner>(ConsumableSpawners);

            for (auto& Spawner : ConsumableSpawners)
                UFortLootPackage::SpawnConsumableActor(Spawner);

            ConsumableSpawners.Free();

            if (AFortAthenaLivingWorldStaticPointProvider::StaticClass())
            {
                TArray<AFortAthenaLivingWorldStaticPointProvider*> Spawners;
                Utils::GetAll<AFortAthenaLivingWorldStaticPointProvider>(Spawners);
                UEAllocatedMap<FName, const UClass*> VehicleSpawnerMap =
                {
                    { FName(L"Athena.Vehicle.SpawnLocation.Motorcycle.Dirtbike"), FindObject<UClass>(L"/Dirtbike/Vehicle/Motorcycle_DirtBike_Vehicle.Motorcycle_DirtBike_Vehicle_C") },
                    { FName(L"Athena.Vehicle.SpawnLocation.Motorcycle.Sportbike"), FindObject<UClass>(L"/Sportbike/Vehicle/Motorcycle_Sport_Vehicle.Motorcycle_Sport_Vehicle_C") },
                    { FName(L"Athena.Vehicle.SpawnLocation.Valet.BasicCar.Taxi"), FindObject<UClass>(L"/Valet/TaxiCab/Valet_TaxiCab_Vehicle.Valet_TaxiCab_Vehicle_C") },
                    { FName(L"Athena.Vehicle.SpawnLocation.Valet.BasicCar.Modded"), FindObject<UClass>(L"/ModdedBasicCar/Vehicle/Valet_BasicCar_Vehicle_SuperSedan.Valet_BasicCar_Vehicle_SuperSedan_C") },
                    { FName(L"Athena.Vehicle.SpawnLocation.Valet.BasicTruck.Upgraded"), FindObject<UClass>(L"/Valet/BasicTruck/Valet_BasicTruck_Vehicle_Upgrade.Valet_BasicTruck_Vehicle_Upgrade_C") },
                    { FName(L"Athena.Vehicle.SpawnLocation.Valet.BigRig.Upgraded"), FindObject<UClass>(L"/Valet/BigRig/Valet_BigRig_Vehicle_Upgrade.Valet_BigRig_Vehicle_Upgrade_C") },
                    { FName(L"Athena.Vehicle.SpawnLocation.Valet.SportsCar.Upgraded"), FindObject<UClass>(L"/Valet/SportsCar/Valet_SportsCar_Vehicle_Upgrade.Valet_SportsCar_Vehicle_Upgrade_C") },
                    { FName(L"Athena.Vehicle.SpawnLocation.Valet.BasicCar.Upgraded"), FindObject<UClass>(L"/Valet/BasicCar/Valet_BasicCar_Vehicle_Upgrade.Valet_BasicCar_Vehicle_Upgrade_C") }
                };

                for (auto& Spawner : Spawners)
                {
                    const UClass* VehicleClass = nullptr;
                    for (int i = 0; i < Spawner->FiltersTags.GameplayTags.Num(); i++)
                    {
                        auto& Tag = Spawner->FiltersTags.GameplayTags.Get(i, FGameplayTag::Size());

                        if (VehicleSpawnerMap.contains(Tag.TagName))
                        {
                            VehicleClass = VehicleSpawnerMap[Tag.TagName];
                            break;
                        }
                    }

                    if (VehicleClass)
                    {
                        auto Vehicle = UWorld::SpawnActor<AFortAthenaVehicle>(VehicleClass, Spawner->K2_GetActorLocation(), Spawner->K2_GetActorRotation());

                        if (auto Car = Vehicle->Cast<AFortDagwoodVehicle>())
                            Car->SetFuel(100.f);
                        //printf("Spawned a %s\n", Spawner->Name.ToString().c_str());
                    }
                    else
                    {
                        for (auto& Tag : Spawner->FiltersTags.GameplayTags)
                            printf("Fix: Tag: %s\n", Tag.TagName.ToString().c_str());
                    }
                }
                Spawners.Free();
            }
            // not an else here because they still use spawners for boats, and fully on s27
            if (VersionInfo.FortniteVersion >= 4.23 && std::floor(VersionInfo.FortniteVersion) != 20 && std::floor(VersionInfo.FortniteVersion) != 21) // its auto on s20 & s21
            {
                TArray<AFortAthenaVehicleSpawner*> Spawners{};
                Utils::GetAll<AFortAthenaVehicleSpawner>(Spawners);

                for (auto& Spawner : Spawners)
                {
                    auto Vehicle = UWorld::SpawnActor<AFortAthenaVehicle>(Spawner->GetVehicleClass(), Spawner->K2_GetActorLocation(), Spawner->K2_GetActorRotation());

                    if (auto Car = Vehicle->Cast<AFortDagwoodVehicle>())
                        Car->SetFuel(100.f);
                }

                Spawners.Free();
            }

            if (VersionInfo.FortniteVersion > 3.4)
            {
                TArray<ABuildingItemCollectorActor*> Collectors{};
                Utils::GetAll<ABuildingItemCollectorActor>(Collectors);
                for (auto& CollectorActor : Collectors)
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

                            TArray<FFortItemEntry*> LootDrops{};

                            UFortLootPackage::ChooseLootForContainer(LootDrops, CollectorActor->DefaultItemLootTierGroupName, Rarity);

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
                Collectors.Free();

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


void AFortGameMode::HandlePostSafeZonePhaseChanged(AFortGameMode* GameMode, int NewSafeZonePhase_Inp)
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

    static auto DurationsOffset = 0;
    if (DurationsOffset == 0)
    {
        DurationsOffset = 0x258;

        if (VersionInfo.FortniteVersion >= 18)
            DurationsOffset = 0x248;
        else if (VersionInfo.FortniteVersion < 15.20)
            DurationsOffset = 0x1f8;
    }

    auto SafeZoneDefinition = &GameState->MapInfo->SafeZoneDefinition;
    TArray<float>& Durations = *(TArray<float>*)(SafeZoneDefinition + DurationsOffset);
    TArray<float>& HoldDurations = *(TArray<float>*)(SafeZoneDefinition + DurationsOffset - 0x10);

    if (VersionInfo.FortniteVersion >= 13.00)
    {


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

        if (!FConfiguration::bLateGame || GameMode->SafeZonePhase > FConfiguration::LateGameZone)
        {
            auto Duration = Durations[NewSafeZonePhase];
            auto HoldDuration = HoldDurations[NewSafeZonePhase];

            GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds + HoldDuration;
            GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
        }
    }

    HandlePostSafeZonePhaseChangedOG(GameMode, NewSafeZonePhase_Inp);

    /*if (FConfiguration::bLateGame && GameMode->SafeZonePhase > FConfiguration::LateGameZone)
    {
        auto newIdx = GameMode->SafeZonePhase - FConfiguration::LateGameZone + 1;
        auto Duration = newIdx >= LateGameDurations.size() ? 0.f : LateGameDurations[newIdx];
        auto HoldDuration = newIdx >= LateGameHoldDurations.size() ? 0.f : LateGameHoldDurations[newIdx];

        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds + HoldDuration;
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
    }*/


    if (FConfiguration::bLateGame && GameMode->SafeZonePhase < FConfiguration::LateGameZone)
    {
        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds;
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + 0.15f;
        return;
    }
    else if (FConfiguration::bLateGame && GameMode->SafeZonePhase == FConfiguration::LateGameZone)
    {
        //auto Duration = Durations[FConfiguration::LateGameZone];
        //auto HoldDuration = HoldDurations[FConfiguration::LateGameZone];

        if (FConfiguration::bLateGame && FConfiguration::bLateGameLongZone)
            GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = 676767.f;
        if (VersionInfo.FortniteVersion >= 13)
            GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Durations[FConfiguration::LateGameZone];
    }

    if (FConfiguration::bLateGame && (SafeZoneLoc.X != 0 || SafeZoneLoc.Y != 0 || SafeZoneLoc.Z != 0))
    {
        GameMode->SafeZoneIndicator->NextCenter = SafeZoneLoc;
        GameMode->SafeZoneIndicator->LastCenter = SafeZoneLoc;
    }

    if (NewSafeZonePhase > (FConfiguration::bLateGame ? FConfiguration::LateGameZone : 1))
    {
        for (auto& UncastedPlayer : GameMode->AlivePlayers)
        {
            auto PlayerController = (AFortPlayerControllerAthena*)UncastedPlayer;

            PlayerController->GetQuestManager(1)->SendStatEvent(PlayerController, EFortQuestObjectiveStatEvent::GetStormPhase(), 1);
        }
    }
}


uint64_t NotifyGameMemberAdded_ = 0;
int16_t WorldPlayerId = 0;
void AFortGameMode::HandleStartingNewPlayer_(UObject* Context, FFrame& Stack)
{
    AFortPlayerControllerAthena* NewPlayer;
    Stack.StepCompiledIn(&NewPlayer);
    Stack.IncrementCode();
    auto GameMode = (AFortGameMode*)Context;
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

        auto& NewMember = GameState->GameMemberInfoArray.Members.Add(*Member, FGameMemberInfo::Size());
        GameState->GameMemberInfoArray.MarkItemDirty(NewMember);

        auto NotifyGameMemberAdded = (void(*)(AFortGameStateAthena*, uint8_t, uint8_t, FUniqueNetIdRepl*)) NotifyGameMemberAdded_;
        if (NotifyGameMemberAdded)
            NotifyGameMemberAdded(GameState, Member->SquadId, Member->TeamIndex, &Member->MemberUniqueId);

        free(Member);
    }

    if (NewPlayer->HasbBuildFree())
        NewPlayer->bBuildFree = FConfiguration::bInfiniteMats;

    if (!NewPlayer->WorldInventory)
    {
        NewPlayer->WorldInventory = UWorld::SpawnActor<AFortInventory>(NewPlayer->WorldInventoryClass, FVector{}, FRotator{}, NewPlayer);
        NewPlayer->WorldInventory->InventoryType = 0;
    }

    PlayerState->WorldPlayerId = WorldPlayerId;

    return callOG(GameMode, Stack.GetCurrentNativeFunction(), HandleStartingNewPlayer, NewPlayer);
}


uint8_t AFortGameMode::PickTeam(AFortGameMode* GameMode, uint8_t PreferredTeam, AFortPlayerControllerAthena* Controller)
{
    if (!GameMode->HasWarmupRequiredPlayerCount())
        return 0;

    uint8_t ret = CurrentTeam;
    auto Playlist = VersionInfo.FortniteVersion >= 3.5 && GameMode->HasWarmupRequiredPlayerCount() ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData) : nullptr;

    printf("Picked team %d %d\n", ret, Playlist ? Playlist->MaxSquadSize : 1);
    if (bIsLargeTeamGame)
    {
        if (CurrentTeam == 4)
            CurrentTeam = 3;
        else
            CurrentTeam = 4;
    }
    else
    {
        if (++PlayersOnCurTeam >= (Playlist ? Playlist->MaxSquadSize : 1))
        {
            CurrentTeam++;
            PlayersOnCurTeam = 0;
        }
    }

    return ret;
}

bool AFortGameMode::StartAircraftPhase(AFortGameMode* GameMode, char a2)
{
    auto Ret = StartAircraftPhaseOG(GameMode, a2);

    auto GameState = (AFortGameStateAthena*)GameMode->GameState;


    auto Playlist = VersionInfo.FortniteVersion >= 3.5 && GameMode->HasWarmupRequiredPlayerCount() ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData) : nullptr;
    if constexpr (FConfiguration::WebhookURL && *FConfiguration::WebhookURL)
    {
        auto curl = curl_easy_init();

        curl_easy_setopt(curl, CURLOPT_URL, FConfiguration::WebhookURL);
        curl_slist* headers = curl_slist_append(NULL, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        char version[6];

        sprintf_s(version, VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "%.2f" : "%.1f", VersionInfo.FortniteVersion);

        auto payload = UEAllocatedString("{\"embeds\": [{\"title\": \"Match has started!\", \"fields\": [{\"name\":\"Version\",\"value\":\"") + version + "\"}, {\"name\":\"Playlist\",\"value\":\"" + (Playlist ? Playlist->PlaylistName.ToString() : "Playlist_DefaultSolo") + "\"},{\"name\":\"Players\",\"value\":\"" + std::to_string(GameMode->AlivePlayers.Num()).c_str() + "\"}], \"color\": " + "\"7237230\", \"footer\": {\"text\":\"Erbium\", \"icon_url\":\"https://cdn.discordapp.com/attachments/1341168629378584698/1436803905119064105/L0WnFa.png.png?ex=6910ef69&is=690f9de9&hm=01a0888b46647959b38ee58df322048ab49e2a5a678e52d4502d9c5e3978d805&\"}, \"timestamp\":\"" + iso8601() + "\"}] }";

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

        curl_easy_perform(curl);

        curl_easy_cleanup(curl);
    }
    GUI::gsStatus = StartedMatch;
    sprintf_s(GUI::windowTitle, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Match started" : (VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "Erbium (FN %.2f, UE %.2f): Match started" : "Erbium (FN %.1f, UE %.2f): Match started"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
    SetConsoleTitleA(GUI::windowTitle);

    // credit to heliato
    if (FConfiguration::bJoinInProgress || (Playlist && (Playlist->HasbAllowJoinInProgress() ? Playlist->bAllowJoinInProgress : false)))
        *(bool*)(uint64_t(&GameMode->WarmupRequiredPlayerCount) - 4) = false;

    if (FConfiguration::bLateGame && VersionInfo.FortniteVersion < 25.20)
    {
        auto Aircraft = GameState->HasAircrafts() ? (GameState->Aircrafts.Num() > 0 ? GameState->Aircrafts[0] : nullptr) : GameState->Aircraft;

        if (!Aircraft)
            return Ret;

        FVector Loc;
        bool bScuffed = false;
        if (GameMode->SafeZoneLocations.Num() < 4)
        {
            bScuffed = true;

            TArray<ABuildingFoundation*> Foundations;
            Utils::GetAll<ABuildingFoundation>(Foundations);
            auto Foundation = Foundations[rand() % Foundations.Num()];

            Foundations.Free();

            SafeZoneLoc = Loc = Foundation->K2_GetActorLocation();

            //FConfiguration::bLateGame = false;
            //printf("LateGame is not supported on this version!\n");
            //return Ret;
        }
        else
        {
            Loc = GameMode->SafeZoneLocations.Get(FConfiguration::LateGameZone + (VersionInfo.FortniteVersion >= 24 ? 3 : 0) - 1, FVector::Size());
        }

        Loc.Z = 17500.f;

        if (GameState->HasDefaultParachuteDeployTraceForGroundDistance())
        {
            GameState->DefaultParachuteDeployTraceForGroundDistance = 2500.f;
        }

        if (Aircraft->HasFlightInfo())
        {
            Aircraft->FlightInfo.FlightSpeed = 0.f;

            Aircraft->FlightInfo.FlightStartLocation = Loc;

            Aircraft->FlightInfo.TimeTillFlightEnd = 7.f;
            Aircraft->FlightInfo.TimeTillDropEnd = 7.f;
            Aircraft->FlightInfo.TimeTillDropStart = 0.f;
        }
        else
        {
            Aircraft->FlightSpeed = 0.f;

            Aircraft->FlightStartLocation = Loc;

            if (Aircraft->HasTimeTillFlightEnd())
            {
                Aircraft->TimeTillFlightEnd = 7.f;
                Aircraft->TimeTillDropEnd = 7.f;
                Aircraft->TimeTillDropStart = 0.f;
            }
        }
        Aircraft->DropStartTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        Aircraft->DropEndTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 7.f;
        Aircraft->FlightStartTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        Aircraft->FlightEndTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 7.f;
        //GameState->bAircraftIsLocked = false;
        //GameState->SafeZonesStartTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 7.6f;
    }

    return Ret;
}


void AFortGameMode::OnAircraftExitedDropZone_(UObject* Context, FFrame& Stack)
{
    AFortAthenaAircraft* Aircraft;
    Stack.StepCompiledIn(&Aircraft);
    Stack.IncrementCode();

    auto GameMode = (AFortGameMode*)Context;
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

AFortSafeZoneIndicator* SetupSafeZoneIndicator(AFortGameMode* GameMode)
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

void StartNewSafeZonePhase(AFortGameMode* GameMode, int NewSafeZonePhase, bool bInitial = false)
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

        auto& SafeZoneState = *(uint8_t*)(__int64(&GameMode->SafeZoneIndicator->FutureReplicator) - 0x4);
        SafeZoneState = 2;

        GameMode->SafeZoneIndicator->OnSafeZoneStateChange(2, false);
        if (GameMode->SafeZoneIndicator->HasSafezoneStateChangedDelegate())
            GameMode->SafeZoneIndicator->SafezoneStateChangedDelegate.Process(GameMode->SafeZoneIndicator, 2);

        if (!bInitial)
            for (auto& UncastedPlayer : GameMode->AlivePlayers)
            {
                auto PlayerController = (AFortPlayerControllerAthena*)UncastedPlayer;

                PlayerController->GetQuestManager(1)->SendStatEvent(PlayerController, EFortQuestObjectiveStatEvent::GetStormPhase(), 1);
            }
    }
}

void (*SpawnInitialSafeZoneOG)(AFortGameMode* GameMode);
void SpawnInitialSafeZone(AFortGameMode* GameMode)
{
    //return;
    GameMode->bSafeZoneActive = true;
    auto SafeZoneIndicator = SetupSafeZoneIndicator(GameMode);

    SafeZoneIndicator->OnSafeZonePhaseChanged.Bind(GameMode, FName(L"HandlePostSafeZonePhaseChanged"));
    GameMode->OnSafeZoneIndicatorSpawned.Process(SafeZoneIndicator);

    StartNewSafeZonePhase(GameMode, FConfiguration::bLateGame ? (FConfiguration::LateGameZone + (VersionInfo.FortniteVersion >= 24 ? 3 : 0)) : 1, true);


    //return SpawnInitialSafeZoneOG(GameMode);
}

void (*UpdateSafeZonesPhaseOG)(AFortGameMode* GameMode);
void UpdateSafeZonesPhase(AFortGameMode* GameMode)
{
    auto& Array = GameMode->SafeZoneIndicator && GameMode->SafeZoneIndicator->HasSafeZonePhases() ? GameMode->SafeZoneIndicator->SafeZonePhases : Phases;
    if (GameMode->bSafeZoneActive && UGameplayStatics::GetTimeSeconds(GameMode) >= GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime && !GameMode->bSafeZonePaused && Array.IsValidIndex(GameMode->SafeZoneIndicator->CurrentPhase + 1))
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

class AFortNavMesh : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortNavMesh);

    DEFINE_PROP(HotSpotManager, const UObject*);
};
void (*OnWorldInitDoneOG)(UNavigationSystem* NavSys, char Mode);
void OnWorldInitDone(UNavigationSystem* NavSys, char Mode)
{
    printf("OnWorldInitDone\n");
    /*NavSys->bAutoCreateNavigationData = true;
    NavSys->bAllowClientSideNavigation = true;
    NavSys->bSupportRebuilding = true;

    OnWorldInitDoneOG(NavSys, Mode);

    auto AllBounds = Utils::GetAll(FindClass("NavMeshBoundsVolume"));
    auto AllNavmeshes = Utils::GetAll<AFortNavMesh>();
    auto HotSpotMgr = TUObjectArray::FindFirstObject("FortAIHotSpotManager");

    //auto Test = (void(*)(UNavigationSystem*)) (ImageBase + 0x1F5C290);
    //Test(NavSys);

    NavSys->OnNavigationBoundsUpdated(AllBounds[0]);
    AllNavmeshes[0]->HotSpotManager = HotSpotMgr;
    //printf("NavGraphData: %llx, AllBounds.Num() = %d\n", NavSys->NavGraphData, AllBounds.Num());
    AllBounds.Free();
    AllNavmeshes.Free();*/
}

void AFortGameMode::Hook()
{
    Utils::ExecHook(GetDefaultObj()->GetFunction("ReadyToStartMatch"), ReadyToStartMatch_, ReadyToStartMatch_OG);
    //if (VersionInfo.EngineVersion == 4.16)
    //    Utils::Hook(Memcury::Scanner::FindPattern("40 55 53 56 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 48 8B 01 48 8B F1").Get(), OnWorldInitDone, OnWorldInitDoneOG);
}

void AFortGameMode::PostLoadHook()
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
    Utils::ExecHook(AFortGameModeAthena::GetDefaultObj()->GetFunction("OnAircraftExitedDropZone"), OnAircraftExitedDropZone_, OnAircraftExitedDropZone_OG);

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
