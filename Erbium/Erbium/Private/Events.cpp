#include "pch.h"
#include "../Public/Events.h"
#include "../../FortniteGame/Public/FortGameMode.h"
#include "../../FortniteGame/Public/BattleRoyaleGamePhaseLogic.h"
#include <thread>

struct FPhaseDataLayerEntry
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FPhaseDataLayerEntry);

    DEFINE_STRUCT_PROP(DataLayerAsset, UObject*);
    DEFINE_STRUCT_PROP(bIsRecursive, bool);
};

struct FPhaseInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FPhaseInfo);

    DEFINE_STRUCT_PROP(DataLayers, TArray<FPhaseDataLayerEntry>);
};

class ASpecialEventScript : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ASpecialEventScript);

    DEFINE_PROP(DelayAfterConentLoad, float);
    DEFINE_PROP(ReplicatedActivePhaseIndex, int);
    DEFINE_PROP(DelayAfterWarmup, float);
    DEFINE_PROP(PhaseInfoArray, TArray<FPhaseInfo>);

    DEFINE_FUNC(OnRep_ReplicatedActivePhaseIndex, void);
};

class ASpecialEventScriptMeshActor : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ASpecialEventScriptMeshActor);

    DEFINE_FUNC(MeshRootStartEvent, void);
    DEFINE_FUNC(OnRep_RootStartTime, void);
};

void Events::StartEvent()
{
    if (VersionInfo.FortniteVersion < 4.4)
        return; // no other events from what i know of?

    auto GameMode = (AFortGameMode*)UWorld::GetWorld()->AuthorityGameMode;
    auto Playlist = VersionInfo.FortniteVersion >= 3.5 && GameMode->HasWarmupRequiredPlayerCount()
        ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData)
        : nullptr;

    for (auto& Event : EventsArray)
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
                // AllLoaders.Free();
            }

        UObject* ScriptingObject = nullptr;
        if (Event.ScriptingClass)
            if (const UClass* ScriptingClass = FindObject<UClass>(Event.ScriptingClass))
            {
                TArray<AActor*> AllLoaders;
                Utils::GetAll(ScriptingClass, AllLoaders);
                ScriptingObject = AllLoaders.Num() > 0 ? AllLoaders[0] : nullptr;
                // AllLoaders.Free();
            }

        for (auto& EventFunction : Event.EventFunctions)
        {
            const UFunction* Function = FindObject<UFunction>(EventFunction.FunctionPath);
            if (!Function)
            {
                printf("[Events] failed to find func: %ls\n", EventFunction.FunctionPath);
                continue;
            }

            UObject* Target = EventFunction.bIsLoaderFunction ? LoaderObject : ScriptingObject;
            if (Target)
            {
                if (wcsstr(EventFunction.FunctionPath, L"OnReady"))
                {
                    struct
                    {
                        UObject* GameState;
                        const UFortPlaylistAthena* Playlist;
                        FGameplayTagContainer PlaylistContextTags;
                    } Params { GameMode->GameState, Playlist, Playlist && Playlist->HasGameplayTagContainer() ? Playlist->GameplayTagContainer : FGameplayTagContainer() };
                    Target->ProcessEvent(const_cast<UFunction*>(Function), &Params);
                }
                else if (wcsstr(EventFunction.FunctionPath, L"StartEventAtIndex"))
                {
                    TArray<ASpecialEventScriptMeshActor*> MeshActors;
                    Utils::GetAll<ASpecialEventScriptMeshActor>(MeshActors);

                    /**static auto GamePhaseOffset = GPL->GetOffset("GamePhase");
                    auto& _GamePhase = *(EAthenaGamePhase*)(__int64(GPL) + GamePhaseOffset);
                    printf("%d\n", _GamePhase);*/
                    if (UFortGameStateComponent_BattleRoyaleGamePhaseLogic::StaticClass())
                    {
                        auto GPL = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameMode);
                        GPL->SetGamePhase(EAthenaGamePhase::SafeZones);
                        GPL->SetGamePhaseStep(EAthenaGamePhaseStep::StormHolding);
                    }
                    else
                    {
                        GameMode->GameState->GamePhase = (uint8)EAthenaGamePhase::SafeZones;
                        GameMode->GameState->GamePhaseStep = (uint8)EAthenaGamePhaseStep::StormHolding;
                    }

                    auto MeshActor = MeshActors[0];

                    auto Scr = (ASpecialEventScript*)Target;
                    
                    Scr->DelayAfterConentLoad = 0.6f;

                    auto MeshNetworkSubsystem = TUObjectArray::FindFirstObject("MeshNetworkSubsystem");

                    if (MeshNetworkSubsystem)
                        *(uint8_t*)(__int64(MeshNetworkSubsystem) + MeshNetworkSubsystem->GetOffset("NodeType")) = 0;

                    MeshActor->MeshRootStartEvent();

                    if (MeshNetworkSubsystem)
                        *(uint8_t*)(__int64(MeshNetworkSubsystem) + MeshNetworkSubsystem->GetOffset("NodeType")) = 2;
                    MeshActor->OnRep_RootStartTime();
                    //Target->Call(const_cast<UFunction*>(Function), 0, 0.f);
                }
                else
                {
                    Target->Call(const_cast<UFunction*>(Function), 0.f);
                }
            }

            if (VersionInfo.FortniteVersion == 8.51 && ScriptingObject && !EventFunction.bIsLoaderFunction)
            {
                std::thread(
                    [ScriptingObject]
                    {
                        std::this_thread::sleep_for(std::chrono::minutes(3));

                        auto SetUnvaultFn = FindObject<UFunction>(L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C.SetUnvaultItemName");
                        auto PillarsFn = FindObject<UFunction>(L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C.PillarsConcluded");
                        FName Name = FName(L"DrumGun");

                        ScriptingObject->ProcessEvent(const_cast<UFunction*>(SetUnvaultFn), &Name);
                        ScriptingObject->ProcessEvent(const_cast<UFunction*>(PillarsFn), &Name);
                    })
                    .detach();
            }
        }

        if (VersionInfo.FortniteVersion >= 16.00)
        {
            for (auto& UncastedPC : GameMode->AlivePlayers)
            {
                auto PlayerController = (AFortPlayerControllerAthena*)UncastedPC;

                auto PickaxeInstance = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search(
                    [&](FFortItemEntry& entry)
                    {
                        return entry.ItemDefinition->Cast<UFortWeaponMeleeItemDefinition>();
                    },
                    FFortItemEntry::Size());

                if (PickaxeInstance)
                    PlayerController->WorldInventory->Remove(PickaxeInstance->ItemGuid);

                auto EventModeActivator = FindObject<UFortItemDefinition>(L"/EventMode/Items/WID_EventMode_Activator.WID_EventMode_Activator");

                PlayerController->WorldInventory->GiveItem(EventModeActivator);
            }
        }

        printf("[Events] Started!\n");
        return;
    }

    printf("[Events] Build does not have an event.\n");
}

void (*ActivatePhaseOG)(ASpecialEventScript* _this, int IndexToActivate, float a3);
void ActivatePhase(ASpecialEventScript* _this, int IndexToActivate, float a3)
{
    // for some reason the 2 functions below dont handle datalayers
    if (VersionInfo.FortniteVersion >= 23)
    {
        // should be in UnloadLevelsAtPhaseEnd
        if (_this->ReplicatedActivePhaseIndex >= 0)
        {
            auto& OldPhaseInfo = _this->PhaseInfoArray.Get(_this->ReplicatedActivePhaseIndex, FPhaseInfo::Size());
            for (int i = 0; i < OldPhaseInfo.DataLayers.Num(); i++)
            {
                auto& DL = OldPhaseInfo.DataLayers.Get(i, FPhaseDataLayerEntry::Size());

                UWorld::GetWorld()->GetDataLayerManager()->SetDataLayerRuntimeState(DL.DataLayerAsset, 0, DL.bIsRecursive);
            }
        }

        // should be in LoadLevelsAtIndex
        auto& PhaseInfo = _this->PhaseInfoArray.Get(IndexToActivate, FPhaseInfo::Size());
        for (int i = 0; i < PhaseInfo.DataLayers.Num(); i++)
        {
            auto& DL = PhaseInfo.DataLayers.Get(i, FPhaseDataLayerEntry::Size());

            UWorld::GetWorld()->GetDataLayerManager()->SetDataLayerRuntimeState(DL.DataLayerAsset, 2, DL.bIsRecursive);
        }
    }

    _this->ReplicatedActivePhaseIndex = IndexToActivate;

    ActivatePhaseOG(_this, IndexToActivate, a3);
}

void Events::Hook()
{
    Utils::Hook(FindActivatePhase(), ActivatePhase, ActivatePhaseOG);
}