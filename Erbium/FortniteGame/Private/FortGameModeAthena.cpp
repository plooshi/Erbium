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

TArray<const UFortAbilitySet*> AbilitySets;

void SetupPlaylist(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState)
{
    static auto Playlist = Utils::FindObject<UFortPlaylistAthena>(FConfiguration::Playlist);

    if (Playlist)
    {
        if (FConfiguration::bForceRespawns)
        {
            Playlist->bRespawnInAir = true;
            Playlist->RespawnHeight.Curve.CurveTable = nullptr;
            Playlist->RespawnHeight.Value = 20000;
            Playlist->RespawnTime.Curve.CurveTable = nullptr;
            Playlist->RespawnTime.Value = 3;
            Playlist->RespawnType = 1; // InfiniteRespawns
            Playlist->bAllowJoinInProgress = true;
        }
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

        if (VersionInfo.FortniteVersion >= 6 && VersionInfo.FortniteVersion < 7)
        {
            if (VersionInfo.FortniteVersion > 6.10)
                ShowFoundation(VersionInfo.FortniteVersion <= 6.21 ? Utils::FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Lake1") : Utils::FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Lake2"));
            else
                ShowFoundation(Utils::FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_StreamingTest12"));

            ShowFoundation(VersionInfo.FortniteVersion <= 6.10 ? Utils::FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_StreamingTest13") : Utils::FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_FloatingIsland"));

            auto IslandScripting = TUObjectArray::FindFirstObject("BP_IslandScripting_C");
            auto UpdateMapOffset = IslandScripting->GetOffset("UpdateMap");
            if (UpdateMapOffset != -1)
            {
                *(bool*)(__int64(IslandScripting) + UpdateMapOffset) = true;
                IslandScripting->ProcessEvent(IslandScripting->GetFunction("OnRep_UpdateMap"), nullptr);
            }
        }

        if (VersionInfo.FortniteVersion >= 8 && VersionInfo.FortniteVersion < 10)
        {
            auto Volcano = Utils::FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.LF_Athena_POI_50x53_Volcano");
            ShowFoundation(Volcano);
        }

        if (VersionInfo.FortniteVersion >= 7 && VersionInfo.FortniteVersion <= 10)
            ShowFoundation(Utils::FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.SLAB_2"));
        else if (VersionInfo.EngineVersion == 4.23) // rest of S10
            ShowFoundation(Utils::FindObject<ABuildingFoundation>(L"/Game/Athena/Maps/Athena_POI_Foundations.Athena_POI_Foundations.PersistentLevel.SLAB_4"));


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
                    __stosb((PBYTE)level, 0, FAdditionalLevelStreamed::Size());
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
                    __stosb((PBYTE)level, 0, FAdditionalLevelStreamed::Size());
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
            NetDriver = ((UNetDriver * (*)(UEngine*, UWorld*, FName)) FindCreateNetDriver())(Engine, World, NetDriverName);

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
            SetWorld(NetDriver, World);
        else
            printf("Failed to listen!");

        free(URL);

        GameMode->WarmupRequiredPlayerCount = 1;

        if (VersionInfo.FortniteVersion >= 4.0)
            SetupPlaylist(GameMode, GameState);

        *Ret = false;
        return;
    }

    if (!GameMode->bWorldIsReady)
    {
        static auto WarmupStartClass = FindClass(FConfiguration::bCreative ? "FortPlayerStartCreative" : "FortPlayerStartWarmup");
        auto Starts = Utils::GetAll(WarmupStartClass);
        auto StartsNum = Starts.Num();
        Starts.Free();
        if (StartsNum == 0 || !GameState->MapInfo)
        {
            *Ret = false;
            return;
        }

        if (VersionInfo.FortniteVersion >= 3.5 && VersionInfo.FortniteVersion <= 4.0)
            SetupPlaylist(GameMode, GameState);
        else if (VersionInfo.EngineVersion >= 4.22 && VersionInfo.EngineVersion < 4.26)
            GameState->OnRep_CurrentPlaylistInfo(); 

        auto AbilitySet = VersionInfo.FortniteVersion >= 8.30 ? Utils::FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer") : Utils::FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_DefaultPlayer.GAS_DefaultPlayer");
        AbilitySets.Add(AbilitySet);

        auto AddToTierData = [&](const UDataTable* Table, UEAllocatedMap<FName, FFortLootTierData*>& TempArr) {
            if (!Table)
                return;

            Table->AddToRoot();
            if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
                for (auto& ParentTable : CompositeTable->ParentTables)
                    for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) ParentTable->RowMap)
                        TempArr[Key] = Val;

            for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) Table->RowMap)
                TempArr[Key] = Val;
        };

        auto AddToPackages = [&](const UDataTable* Table, UEAllocatedMap<FName, FFortLootPackageData*>& TempArr) {
            if (!Table)
                return;

            Table->AddToRoot();
            if (auto CompositeTable = Table->Cast<UCompositeDataTable>())
                for (auto& ParentTable : CompositeTable->ParentTables)
                    for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>) ParentTable->RowMap)
                        TempArr[Key] = Val;

            for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>) Table->RowMap)
                TempArr[Key] = Val;
        };


        auto Playlist = Utils::FindObject<UFortPlaylistAthena>(FConfiguration::Playlist);

        UEAllocatedMap<FName, FFortLootTierData*> LootTierDataTempArr;
        auto LootTierData = Playlist ? Playlist->LootTierData.Get() : nullptr;
        if (!LootTierData)
            LootTierData = Utils::FindObject<UDataTable>(L"/Game/Items/Datatables/AthenaLootTierData_Client.AthenaLootTierData_Client");
        if (LootTierData)
            AddToTierData(LootTierData, LootTierDataTempArr);

        for (auto& [_, Val] : LootTierDataTempArr)
            TierDataAllGroups.Add(Val);

        UEAllocatedMap<FName, FFortLootPackageData*> LootPackageTempArr;
        auto LootPackages = Playlist ? Playlist->LootPackages.Get() : nullptr;
        if (!LootPackages) 
            LootPackages = Utils::FindObject<UDataTable>(L"/Game/Items/Datatables/AthenaLootPackages_Client.AthenaLootPackages_Client");
        if (LootPackages)
            AddToPackages(LootPackages, LootPackageTempArr);

        for (auto& [_, Val] : LootPackageTempArr)
            LPGroupsAll.Add(Val);

        if (VersionInfo.FortniteVersion >= 21)
        {
            AbilitySets.Add(Utils::FindObject<UFortAbilitySet>(L"/TacticalSprintGame/Gameplay/AS_TacticalSprint.AS_TacticalSprint"));
            AbilitySets.Add(Utils::FindObject<UFortAbilitySet>(L"/Ascender/Gameplay/Ascender/AS_Ascender.AS_Ascender"));
        }

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

                    auto& LootTableData = GetFromOffset<FFortGameFeatureLootTableData>(Object, DefaultLootTableDataOffset);
                    auto LTDFeatureData = LootTableData.LootTierData.Get();
                    auto LootPackageData = LootTableData.LootPackageData.Get();

                    if (LTDFeatureData)
                    {
                        UEAllocatedMap<FName, FFortLootTierData*> LTDTempData;

                        AddToTierData(LTDFeatureData, LTDTempData);

                        /*for (auto& Tag : Playlist->GameplayTagContainer.GameplayTags)
                            for (auto& Override : Object->PlaylistOverrideLootTableData)
                                if (Tag.TagName == Override.First.TagName)
                                    AddToTierData(Override.Second.LootTierData.Get(), LTDTempData);*/

                        for (auto& [_, Val] : LTDTempData)
                            TierDataAllGroups.Add(Val);
                    }

                    if (LootPackageData)
                    {
                        UEAllocatedMap<FName, FFortLootPackageData*> LPTempData;

                        AddToPackages(LootPackageData, LPTempData);

                        /*for (auto& Tag : Playlist->GameplayTagContainer.GameplayTags)
                            for (auto& Override : Object->PlaylistOverrideLootTableData)
                                if (Tag.TagName == Override.First.TagName)
                                    AddToPackages(Override.Second.LootPackageData.Get(), LPTempData);*/

                        for (auto& [_, Val] : LPTempData)
                            LPGroupsAll.Add(Val);
                    }
                }
            }


        UFortLootPackage::SpawnFloorLootForContainer(Utils::FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C"));
        UFortLootPackage::SpawnFloorLootForContainer(Utils::FindObject<UClass>(L"/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C"));

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

        const UObject* BattleBusDef = nullptr;
        const UClass* SupplyDropClass = nullptr;
        if (VersionInfo.FortniteVersion == 18.40)
            BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_HeadbandBus.BBID_HeadbandBus");
        else if (VersionInfo.FortniteVersion == 1.11 || VersionInfo.FortniteVersion == 7.30 || VersionInfo.FortniteVersion == 11.31 || VersionInfo.FortniteVersion == 15.10 || VersionInfo.FortniteVersion == 19.01)
        {
            BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_WinterBus.BBID_WinterBus");
            SupplyDropClass = Utils::FindObject<UClass>(L"/Game/Athena/SupplyDrops/AthenaSupplyDrop_Holiday.AthenaSupplyDrop_Holiday_C");
        }
        else if (VersionInfo.FortniteVersion == 5.10 || VersionInfo.FortniteVersion == 9.41 || VersionInfo.FortniteVersion == 14.20 || VersionInfo.FortniteVersion == 18.00)
        {
            if (VersionInfo.FortniteVersion == 5.10)
                BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus.BBID_BirthdayBus");
            else if (VersionInfo.FortniteVersion == 9.41)
                BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus2nd.BBID_BirthdayBus2nd");
            else if (VersionInfo.FortniteVersion == 14.20)
                BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus3rd.BBID_BirthdayBus3rd");
            else
                BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BirthdayBus4th.BBID_BirthdayBus4th");

            SupplyDropClass = Utils::FindObject<UClass>(L"/Game/Athena/SupplyDrops/AthenaSupplyDrop_BDay.AthenaSupplyDrop_BDay_C");
        }
        else if (VersionInfo.FortniteVersion == 1.8 || VersionInfo.FortniteVersion == 6.20 || VersionInfo.FortniteVersion == 6.21 || VersionInfo.FortniteVersion == 11.10 || VersionInfo.FortniteVersion == 14.40 || VersionInfo.FortniteVersion == 18.21)
            BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_HalloweenBus.BBID_HalloweenBus");
        else if (VersionInfo.FortniteVersion == 14.30)
            BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BusUpgrade1.BBID_BusUpgrade1");
        else if (VersionInfo.FortniteVersion == 14.50)
            BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BusUpgrade2.BBID_BusUpgrade2");
        else if (VersionInfo.FortniteVersion == 14.60)
            BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BusUpgrade3.BBID_BusUpgrade3");
        else if (VersionInfo.FortniteVersion >= 12.30 && VersionInfo.FortniteVersion <= 12.61)
        {
            BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_DonutBus.BBID_DonutBus");
            BattleBusDef = Utils::FindObject<UClass>(L"/Game/Athena/SupplyDrops/AthenaSupplyDrop_Donut.AthenaSupplyDrop_Donut_C");
        }
        else if (VersionInfo.FortniteVersion == 9.30)
            BattleBusDef = Utils::FindObject<UObject>(L"/Game/Athena/Items/Cosmetics/BattleBuses/BBID_WorldCupBus.BBID_WorldCupBus");

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

        if (SupplyDropClass)
            for (auto& Info : GameState->MapInfo->SupplyDropInfoList)
                Info->SupplyDropClass = SupplyDropClass;


        if (VersionInfo.FortniteVersion > 3.4)
        {
            const UCurveTable* GameData = Playlist ? Playlist->GameData : nullptr;
            if (!GameData)
                GameData = Utils::FindObject<UCurveTable>(L"/Game/Athena/Balance/DataTables/AthenaGameData.AthenaGameData");

            UEAllocatedMap<int, float> WeightMap;
            float Sum = 0;
            float Weight;
            auto VMGroup = UKismetStringLibrary::Conv_StringToName(FString(L"Loot_AthenaVending"));

            for (int i = 0; i < 6; i++)
            {
                float Weight;
                UDataTableFunctionLibrary::EvaluateCurveTableRow(GameState->MapInfo->VendingMachineRarityCount.Curve.CurveTable, GameState->MapInfo->VendingMachineRarityCount.Curve.RowName, (float)i, nullptr, &Weight, FString());

                WeightMap[i] = Weight;
                Sum += Weight;
            }

            UDataTableFunctionLibrary::EvaluateCurveTableRow(GameState->MapInfo->VendingMachineRarityCount.Curve.CurveTable, GameState->MapInfo->VendingMachineRarityCount.Curve.RowName, 0.f, nullptr, &Weight, FString());

            float TotalWeight = std::accumulate(WeightMap.begin(), WeightMap.end(), 0.0f, [&](float acc, const std::pair<int, float>& p) { return acc + p.second; });
            for (auto& VendingMachine : Utils::GetAll<ABuildingItemCollectorActor>())
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
                        VendingMachine->K2_DestroyActor();
                        continue;
                    }

                    int AttemptsToGetItem = 0;
                    for (int i = 0; i < VendingMachine->ItemCollections.Num(); i++)
                    {
                        if (AttemptsToGetItem > 5)
                        {
                            AttemptsToGetItem = 0;
                            goto PickNum;
                        }

                        auto& Collection = VendingMachine->ItemCollections.Get(i, FCollectorUnitInfo::Size());

                        auto LootDrops = UFortLootPackage::ChooseLootForContainer(VMGroup, Rarity);

                        if (Collection.OutputItemEntry.Num() > 0)
                        {
                            Collection.OutputItemEntry.ResetNum();
                            Collection.OutputItem = nullptr;
                        }

                        for (auto& LootDrop : LootDrops)
                        {
                            if (AFortInventory::IsPrimaryQuickbar(LootDrop->ItemDefinition))
                            {
                                bool AlreadyInCollections = false;

                                for (int q = 0; q < VendingMachine->ItemCollections.Num(); q++)
                                {
                                    auto& Coll = VendingMachine->ItemCollections.Get(q, FCollectorUnitInfo::Size());

                                    if (Coll.OutputItem == LootDrop->ItemDefinition)
                                        AlreadyInCollections = true;
                                }

                                if (AlreadyInCollections)
                                    goto PickNum;

                                Collection.OutputItem = LootDrop->ItemDefinition;
                            }

                            Collection.OutputItemEntry.Add(*LootDrop, FFortItemEntry::Size());
                            free(LootDrop);
                        }

                        if (!Collection.OutputItem)
                        {
                            i--;
                            AttemptsToGetItem++;

                            continue;
                        }

                        UEAllocatedWString RowName = L"Default.VendingMachine.Cost.";
                        switch (i)
                        {
                        case 0:
                            RowName += L"Wood";
                            break;
                        case 1:
                            RowName += L"Stone";
                            break;
                        case 2:
                            RowName += L"Metal";
                            break;
                        }

                        Collection.InputCount.Curve.CurveTable = GameData;
                        Collection.InputCount.Curve.RowName = UKismetStringLibrary::Conv_StringToName(FString(RowName.c_str()));
                        Collection.InputCount.Value = (float)(Rarity - 1);
                    }

                    VendingMachine->StartingGoalLevel = Rarity;
                }
                else
                    VendingMachine->K2_DestroyActor();
            }
        }
        GameMode->DefaultPawnClass = Utils::FindObject<UClass>(L"/Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C");

        if (VersionInfo.EngineVersion == 4.16)
        {
            auto sRef = Memcury::Scanner::FindStringRef(L"CollectGarbageInternal() is flushing async loading").Get();
            uint64_t CollectGarbage = 0;

            if (sRef)
            {
                for (int i = 0; i < 1000; i++) {
                    auto Ptr = (uint8_t*)(sRef - i);

                    if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C) {
                        CollectGarbage = uint64_t(Ptr);
                        break;
                    }
                    else if (*Ptr == 0x40 && *(Ptr + 1) == 0x55) {
                        CollectGarbage = uint64_t(Ptr);
                        break;
                    }
                    else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4) {
                        CollectGarbage = uint64_t(Ptr);
                        break;
                    }
                }

                Utils::Patch<uint8_t>(CollectGarbage, 0xC3);
            }
        }
        char buffer[67];
        sprintf_s(buffer, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Joinable" : (VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "Erbium (FN %.2f, UE %.2f): Joinable" : "Erbium (FN %.1f, UE %.2f): Joinable"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
        SetConsoleTitleA(buffer);
        GameMode->bWorldIsReady = true;
    }

    *Ret = VersionInfo.EngineVersion < 4.24 ? callOGWithRet(GameMode, Stack.GetCurrentNativeFunction(), ReadyToStartMatch) : GameMode->AlivePlayers.Num() >= GameMode->WarmupRequiredPlayerCount;
    if (VersionInfo.FortniteVersion >= 11.00 && !*Ret)
    {
        auto Time = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        auto WarmupDuration = 60.f;

        GameState->WarmupCountdownStartTime = Time;
        GameState->WarmupCountdownEndTime = Time + WarmupDuration;
        GameMode->WarmupCountdownDuration = WarmupDuration;
        GameMode->WarmupEarlyCountdownDuration = WarmupDuration;
    }
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
        static auto SmartItemDefClass = FindClass("FortSmartBuildingItemDefinition");
        static bool HasCosmeticLoadoutPC = NewPlayer->HasCosmeticLoadoutPC();
        static bool HasCustomizationLoadout = NewPlayer->HasCustomizationLoadout();

        if (HasCosmeticLoadoutPC && NewPlayer->CosmeticLoadoutPC.Pickaxe)
            NewPlayer->WorldInventory->GiveItem(NewPlayer->CosmeticLoadoutPC.Pickaxe->WeaponDefinition);
        else if (HasCustomizationLoadout && NewPlayer->CustomizationLoadout.Pickaxe)
            NewPlayer->WorldInventory->GiveItem(NewPlayer->CustomizationLoadout.Pickaxe->WeaponDefinition);
        else
        {
            static auto DefaultPickaxe = Utils::FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");

            NewPlayer->WorldInventory->GiveItem(DefaultPickaxe);
        }

        for (int i = 0; i < GameMode->StartingItems.Num(); i++)
        {
            auto& StartingItem = GameMode->StartingItems.Get(i, FItemAndCount::Size());

            if (StartingItem.Count && (!SmartItemDefClass || !StartingItem.Item->IsA(SmartItemDefClass)))
                NewPlayer->WorldInventory->GiveItem(StartingItem.Item, StartingItem.Count);
        }
        
        for (auto& AbilitySet : AbilitySets)
            NewPlayer->PlayerState->AbilitySystemComponent->GiveAbilitySet(AbilitySet);

        static auto ApplyCharacterCustomization = FindApplyCharacterCustomization();

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
    }
    else
    {
        //NewPlayer->WorldInventory->Inventory.ReplicatedEntries.ResetNum();
        //NewPlayer->WorldInventory->Inventory.ItemInstances.ResetNum();
        static auto AmmoClass = FindClass("FortAmmoItemDefinition");
        static auto ResourceClass = FindClass("FortResourceItemDefinition");

        UEAllocatedVector<FGuid> GuidsToRemove;
        for (int i = 0; i < NewPlayer->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            auto& Entry = NewPlayer->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

            if (AFortInventory::IsPrimaryQuickbar(Entry.ItemDefinition) || Entry.ItemDefinition->IsA(AmmoClass) || Entry.ItemDefinition->IsA(ResourceClass))
            {
                //NewPlayer->WorldInventory->Inventory.ReplicatedEntries.Remove(i, FFortItemEntry::Size());
                //i--;
                GuidsToRemove.push_back(Entry.ItemGuid);
            }
        }

        for (auto& Guid : GuidsToRemove)
            NewPlayer->WorldInventory->Remove(Guid);

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


        if (FConfiguration::bLateGame && Pawn && GameState->Aircrafts.Num() > 0 && GameState->Aircrafts[0])
        {
            FVector AircraftLocation = GameState->Aircrafts[0]->K2_GetActorLocation();

            float Angle = (float)rand() / 5215.03002625f;
            float Radius = (float)(rand() % 1000);

            float OffsetX = cosf(Angle) * Radius;
            float OffsetY = sinf(Angle) * Radius;

            FVector Offset;
            Offset.X = OffsetX;
            Offset.Y = OffsetY;
            Offset.Z = 0.0f;

            FVector NewLoc = AircraftLocation + Offset;

            Pawn->K2_SetActorLocation(NewLoc, false, nullptr, false);

            Pawn->SetShield(100.f);

            auto Shotgun = LateGame::GetShotgun();
            auto AssaultRifle = LateGame::GetAssaultRifle();
            auto Sniper = LateGame::GetSniper();
            auto Heal = LateGame::GetHeal();
            auto HealSlot2 = LateGame::GetHeal();

            int ShotgunClipSize = AFortInventory::GetStats((UFortWeaponItemDefinition*)Shotgun.Item)->ClipSize;
            int AssaultRifleClipSize = AFortInventory::GetStats((UFortWeaponItemDefinition*)AssaultRifle.Item)->ClipSize;
            int SniperClipSize = AFortInventory::GetStats((UFortWeaponItemDefinition*)Sniper.Item)->ClipSize;
            // for grappler
            int HealClipSize = Heal.Item->IsA<UFortWeaponItemDefinition>() ? AFortInventory::GetStats((UFortWeaponItemDefinition*)Heal.Item)->ClipSize : 0;
            int HealSlot2ClipSize = HealSlot2.Item->IsA<UFortWeaponItemDefinition>() ? AFortInventory::GetStats((UFortWeaponItemDefinition*)HealSlot2.Item)->ClipSize : 0;

            NewPlayer->WorldInventory->GiveItem(LateGame::GetResource(EFortResourceType::GetWood()), 500);
            NewPlayer->WorldInventory->GiveItem(LateGame::GetResource(EFortResourceType::GetStone()), 500);
            NewPlayer->WorldInventory->GiveItem(LateGame::GetResource(EFortResourceType::GetMetal()), 500);

            NewPlayer->WorldInventory->GiveItem(LateGame::GetAmmo(EAmmoType::Assault), 250);
            NewPlayer->WorldInventory->GiveItem(LateGame::GetAmmo(EAmmoType::Shotgun), 50);
            NewPlayer->WorldInventory->GiveItem(LateGame::GetAmmo(EAmmoType::Submachine), 400);
            NewPlayer->WorldInventory->GiveItem(LateGame::GetAmmo(EAmmoType::Rocket), 6);
            NewPlayer->WorldInventory->GiveItem(LateGame::GetAmmo(EAmmoType::Sniper), 20);

            NewPlayer->WorldInventory->GiveItem(Shotgun.Item, Shotgun.Count, ShotgunClipSize);
            NewPlayer->WorldInventory->GiveItem(AssaultRifle.Item, AssaultRifle.Count, AssaultRifleClipSize);
            NewPlayer->WorldInventory->GiveItem(Sniper.Item, Sniper.Count, SniperClipSize);
            NewPlayer->WorldInventory->GiveItem(Heal.Item, Heal.Count, HealClipSize);
            NewPlayer->WorldInventory->GiveItem(HealSlot2.Item, HealSlot2.Count, HealSlot2ClipSize);
        }

        static bool bMatchStarted = false;

        if (!bMatchStarted)
        {
            bMatchStarted = true;
            auto GameState = (AFortGameStateAthena*)GameMode->GameState;

            char buffer[67];
            sprintf_s(buffer, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Match started" : (VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "Erbium (FN %.2f, UE %.2f): Match started" : "Erbium (FN %.1f, UE %.2f): Match started"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
            SetConsoleTitleA(buffer);
        }
    }

    *Ret = Pawn;
}


void AFortGameModeAthena::HandlePostSafeZonePhaseChanged(AFortGameModeAthena* GameMode, int NewSafeZonePhase_Inp)
{
    if (!GameMode->SafeZoneIndicator)
        return;

    printf("call\n");
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    
    auto NewSafeZonePhase = NewSafeZonePhase_Inp >= 0 ? NewSafeZonePhase_Inp : GameMode->SafeZonePhase + 1;

    constexpr static std::array<float, 8> LateGameDurations{
        0.f,
        65.f,
        60.f,
        50.f,
        45.f,
        35.f,
        30.f,
        40.f,
    };

    constexpr static std::array<float, 8> LateGameHoldDurations{
        0.f,
        60.f,
        55.f,
        50.f,
        45.f,
        30.f,
        0.f,
        0.f,
    };

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


        static bool bSetDurations = false;
        if (!bSetDurations)
        {
            bSetDurations = true;

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

        if (!FConfiguration::bLateGame)
        {
            auto Duration = Durations[NewSafeZonePhase];
            auto HoldDuration = HoldDurations[NewSafeZonePhase];
            printf("new dur %f\n", Duration);
            printf("new holddur %f\n", HoldDuration);

            GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + HoldDuration;
            GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
        }
    } 

    HandlePostSafeZonePhaseChangedOG(GameMode, NewSafeZonePhase_Inp);

    if (FConfiguration::bLateGame && GameMode->SafeZonePhase > 3)
    {
        auto Duration = LateGameDurations[NewSafeZonePhase];
        auto HoldDuration = LateGameHoldDurations[NewSafeZonePhase];

        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + HoldDuration;
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
    }


    if (FConfiguration::bLateGame && GameMode->SafeZonePhase < 3)
    {
        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = (float) UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + 0.15f;
    }
    else if (FConfiguration::bLateGame && GameMode->SafeZonePhase == 3)
    {
        auto Duration = LateGameDurations[GameMode->SafeZonePhase - 1];

        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = (float) UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 30.f;
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
    }
}


void AFortGameModeAthena::HandleStartingNewPlayer_(UObject* Context, FFrame& Stack) {
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

    if (NewPlayer->HasbBuildFree())
        NewPlayer->bBuildFree = FConfiguration::bInfiniteMats;

    return callOG(GameMode, Stack.GetCurrentNativeFunction(), HandleStartingNewPlayer, NewPlayer);
}


uint8_t AFortGameModeAthena::PickTeam(AFortGameModeAthena* GameMode, uint8_t PreferredTeam, AFortPlayerControllerAthena* Controller) {
    uint8_t ret = CurrentTeam;

    auto Playlist = VersionInfo.FortniteVersion >= 4.0 ? GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData : nullptr;
    if (++PlayersOnCurTeam >= (Playlist ? Playlist->MaxSquadSize : 1))
    {
        CurrentTeam++;
        PlayersOnCurTeam = 0;
    }

    return ret;
}

bool AFortGameModeAthena::StartAircraftPhase(AFortGameModeAthena* GameMode, char a2)
{
    auto Ret = StartAircraftPhaseOG(GameMode, a2);

    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    if (FConfiguration::bLateGame)
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
            printf("LateGame is not supported on this version!\n");
            return Ret;
        }
        FVector Loc = GameMode->SafeZoneLocations.Get(3, FVector::Size());
        Loc.Z = 17500.f;
        
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
        GameState->SafeZonesStartTime = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 8.f;
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

void AFortGameModeAthena::Hook()
{
    auto spdf = GetDefaultObj()->GetFunction("SpawnDefaultPawnFor");
    SpawnDefaultPawnForIdx = spdf->GetVTableIndex();
    Utils::ExecHook(GetDefaultObj()->GetFunction("ReadyToStartMatch"), ReadyToStartMatch_, ReadyToStartMatch_OG);
    Utils::ExecHook(spdf, SpawnDefaultPawnFor);
    Utils::Hook(FindHandlePostSafeZonePhaseChanged(), HandlePostSafeZonePhaseChanged, HandlePostSafeZonePhaseChangedOG);
    Utils::ExecHook(GetDefaultObj()->GetFunction("HandleStartingNewPlayer"), HandleStartingNewPlayer_, HandleStartingNewPlayer_OG);
    Utils::Hook(FindPickTeam(), PickTeam, PickTeamOG);
    Utils::Hook(FindStartAircraftPhase(), StartAircraftPhase, StartAircraftPhaseOG);
    Utils::ExecHook(GetDefaultObj()->GetFunction("OnAircraftExitedDropZone"), OnAircraftExitedDropZone_, OnAircraftExitedDropZone_OG);
}
