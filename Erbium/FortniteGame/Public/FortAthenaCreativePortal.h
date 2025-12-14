#pragma once
#include "../../pch.h"
#include "FortPlayerControllerAthena.h"

class UFortCreativeVolumeLinkComponent : public UActorComponent
{
public:
    UCLASS_COMMON_MEMBERS(UFortCreativeVolumeLinkComponent);
};

class AFortMinigameSettingsBuilding : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortMinigameSettingsBuilding);
    
    DEFINE_PROP(SettingsVolume, AActor*);
    DEFINE_PROP(CreativeLinkComponent, UFortCreativeVolumeLinkComponent*);

    DEFINE_FUNC(OnRep_SettingsVolume, void);
    
    DefHookOg(void, BeginPlay, AFortMinigameSettingsBuilding* Settings);
    
    InitHooks;
};

class AMinigameSettingsMachine_C : public AFortMinigameSettingsBuilding
{
public:
    UCLASS_COMMON_MEMBERS(AMinigameSettingsMachine_C);
};

class AFortAthenaCreativePortal : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaCreativePortal);

    DEFINE_PROP(OwningPlayer, FUniqueNetIdRepl);
    DEFINE_PROP(bPortalOpen, bool);
    DEFINE_PROP(PlayersReady, TArray<FUniqueNetIdRepl>);
    DEFINE_PROP(bIsPublishedPortal, bool);
    DEFINE_PROP(bUserInitiatedLoad, bool);
    DEFINE_PROP(bInErrorState, bool);
    DEFINE_PROP(LinkedVolume, AFortVolume*);
    DEFINE_PROP(CurrentPopulation, int32);
    DEFINE_PROP(MaxAvailablePopulation, int32);

    DEFINE_FUNC(OnRep_OwningPlayer, void);
    DEFINE_FUNC(OnRep_PortalOpen, void);
    DEFINE_FUNC(OnRep_PlayersReady, void);
    DEFINE_FUNC(OnRep_PublishedPortal, void);
    DEFINE_FUNC(OnRep_IslandInfo, void);
    DEFINE_FUNC(SetCurrentPlayset, void);
    DEFINE_FUNC(OnRep_PopulationChanged, void);

    static AFortAthenaCreativePortal* Create(AFortPlayerControllerAthena* Player);
    static void TeleportPlayerToLinkedVolume(UObject*, FFrame&);

    InitHooks;
};