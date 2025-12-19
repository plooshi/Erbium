#pragma once
#include "../../pch.h"
#include "FortPlaylistAthena.h"
#include "../../Engine/Public/CurveTable.h"
#include "FortPlayerStateAthena.h"
#include "LevelStreamingDynamic.h"

struct FGameMemberInfo : public FFastArraySerializerItem
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameMemberInfo);

    DEFINE_STRUCT_PROP(TeamIndex, uint8);
    DEFINE_STRUCT_PROP(SquadId, uint8);
    DEFINE_STRUCT_PROP(MemberUniqueId, FUniqueNetIdRepl);
};

struct FGameMemberInfoArray : public FFastArraySerializer
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameMemberInfoArray);

    DEFINE_STRUCT_PROP(Members, TArray<FGameMemberInfo>);
};

struct FAdditionalLevelStreamed
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FAdditionalLevelStreamed);

    DEFINE_STRUCT_PROP(LevelName, FName);
    DEFINE_STRUCT_PROP(bIsServerOnly, bool);
};

struct FPlaylistPropertyArray : public FFastArraySerializer
{
    USCRIPTSTRUCT_COMMON_MEMBERS(FPlaylistPropertyArray);

    DEFINE_STRUCT_PROP(PlaylistReplicationKey, int32);
    DEFINE_STRUCT_PROP(BasePlaylist, const UFortPlaylistAthena*);
    DEFINE_STRUCT_PROP(OverridePlaylist, const UFortPlaylistAthena*);
};

class UFortSupplyDropInfo : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortSupplyDropInfo);

    DEFINE_PROP(SupplyDropClass, TSubclassOf<UObject>);
};

struct FFortSafeZoneDefinition
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortSafeZoneDefinition);

    DEFINE_STRUCT_PROP(Count, FScalableFloat);
    DEFINE_STRUCT_PROP(Radius, FScalableFloat);
    DEFINE_STRUCT_PROP(WaitTime, FScalableFloat);
    DEFINE_STRUCT_PROP(ShrinkTime, FScalableFloat);
    DEFINE_STRUCT_PROP(PlayerCapSolo, FScalableFloat);
    DEFINE_STRUCT_PROP(MegaStormGridCellThickness, FScalableFloat);
};

struct FAircraftFlightInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FAircraftFlightInfo);
    uint8_t Padding[0x50];

    DEFINE_STRUCT_PROP(FlightSpeed, float);
    DEFINE_STRUCT_PROP(FlightStartLocation, FVector);
    DEFINE_STRUCT_PROP(TimeTillFlightEnd, float);
    DEFINE_STRUCT_PROP(TimeTillDropStart, float);
    DEFINE_STRUCT_PROP(TimeTillDropEnd, float);
};

class AFortAthenaAircraft : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaAircraft);

    DEFINE_PROP(FlightInfo, FAircraftFlightInfo);
    DEFINE_PROP(FlightStartTime, float);
    DEFINE_PROP(FlightEndTime, float);
    DEFINE_PROP(FlightSpeed, float);
    DEFINE_PROP(FlightStartLocation, FVector);
    DEFINE_PROP(TimeTillFlightEnd, float);
    DEFINE_PROP(TimeTillDropStart, float);
    DEFINE_PROP(TimeTillDropEnd, float);
    DEFINE_PROP(DefaultBusSkin, const UObject*);
    DEFINE_PROP(SpawnedCosmeticActor, const UObject*);
    DEFINE_PROP(FlightElapsedTime, float);
    DEFINE_PROP(DropStartTime, float);
    DEFINE_PROP(DropEndTime, float);
    DEFINE_PROP(ReplicatedFlightTimestamp, float);

    DEFINE_STATIC_FUNC(SpawnAircraft, AFortAthenaAircraft*);
};


struct FBoxSphereBounds final
{
public:
    struct FVector                                Origin;                                            // 0x0000(0x0018)(Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
    struct FVector                                BoxExtent;                                         // 0x0018(0x0018)(Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
    double                                        SphereRadius;                                      // 0x0030(0x0008)(Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};


class AFortAthenaMapInfo : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaMapInfo);

    DEFINE_PROP(SupplyDropInfoList, TArray<UFortSupplyDropInfo*>);
    DEFINE_PROP(VendingMachineRarityCount, FScalableFloat);
    DEFINE_PROP(SafeZoneDefinition, FFortSafeZoneDefinition);
    DEFINE_PROP(AircraftClass, TSubclassOf<AFortAthenaAircraft>);
    DEFINE_PROP(FlightInfos, TArray<FAircraftFlightInfo>);
    DEFINE_PROP(CachedPlayableBoundsForClients, FBoxSphereBounds);
    DEFINE_PROP(LlamaQuantityMin, FScalableFloat);
    DEFINE_PROP(LlamaQuantityMax, FScalableFloat);
    DEFINE_PROP(LlamaClass, UClass*);
    DEFINE_PROP(SupplyDropClass, TSubclassOf<UObject>);

    DEFINE_FUNC(GetMapCenter, FVector);
};

class AFortVolumeManager : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortVolumeManager);

    DEFINE_BITFIELD_PROP(bInSpawningStartup);
};

struct FPlaylistStreamedLevelData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FPlaylistStreamedLevelData);
    ULevelStreamingDynamic* StreamingLevel;

    DEFINE_STRUCT_PROP(bIsFinishedStreaming, bool);
};


class AFortCreativePortalManager : public AActor
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(AFortCreativePortalManager);

    DEFINE_PROP(AllPortals, TArray<AActor*>);
    DEFINE_PROP(UsedPortals, TArray<AActor*>);
    DEFINE_PROP(AvailablePortals, TArray<AActor*>);
};

class UFortMissionInfo : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortMissionInfo);

    DEFINE_BITFIELD_PROP(bStartPlayingOnLoad);
};

class UFortMissionLibrary : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortMissionLibrary);

    DEFINE_STATIC_FUNC(LoadMission, void);
};

class UNavigationSystem : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UNavigationSystem);

    DEFINE_BITFIELD_PROP(bAutoCreateNavigationData);
    DEFINE_BITFIELD_PROP(bAllowClientSideNavigation);
    DEFINE_BITFIELD_PROP(bSupportRebuilding);
    DEFINE_PROP(NavGraphData, UObject*);

    DEFINE_FUNC(OnNavigationBoundsUpdated, void);
};

class UFortServerBotManagerAthena : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortServerBotManagerAthena);

    DEFINE_PROP(CachedGameState, AActor*);
    DEFINE_PROP(CachedGameMode, AActor*);
    DEFINE_PROP(CachedBotMutator, AActor*);
};

class AFortGameStateAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortGameStateAthena);
    static inline std::unordered_map<int32, TSubclassOf<AActor>> BuildingClassMap;

    DEFINE_PROP(CurrentPlaylistInfo, FPlaylistPropertyArray);
    DEFINE_PROP(CurrentPlaylistId, int32);
    DEFINE_PROP(MapInfo, AFortAthenaMapInfo*);
    DEFINE_PROP(AdditionalPlaylistLevelsStreamed, TArray<FAdditionalLevelStreamed>);
    DEFINE_PROP(DefaultParachuteDeployTraceForGroundDistance, float);
    DEFINE_PROP(AthenaGameDataTable, UCurveTable*);
    DEFINE_PROP(CurrentPlaylistData, const UFortPlaylistAthena*);
    DEFINE_PROP(GameMemberInfoArray, FGameMemberInfoArray);
    DEFINE_PROP(AllPlayerBuildableClassesIndexLookup, TMap<TSubclassOf<AActor>, int32>);
    DEFINE_PROP(AllPlayerBuildableClasses, TArray<TSubclassOf<AActor>>);
    DEFINE_PROP(WorldLevel, int32);
    DEFINE_PROP(WarmupCountdownStartTime, float);
    DEFINE_PROP(WarmupCountdownEndTime, float);
    DEFINE_PROP(PlayersLeft, int32);
    DEFINE_PROP(WinningTeam, int32);
    DEFINE_PROP(WinningPlayerState, AFortPlayerStateAthena*);
    DEFINE_PROP(GamePhase, uint8);
    DEFINE_PROP(GamePhaseStep, uint8);
    DEFINE_PROP(Aircrafts, TArray<AFortAthenaAircraft*>);
    DEFINE_PROP(Aircraft, AFortAthenaAircraft*);
    DEFINE_PROP(SafeZonesStartTime, float);
    DEFINE_PROP(AirCraftBehavior, uint8);
    DEFINE_PROP(CachedSafeZoneStartUp, uint8);
    DEFINE_PROP(DefaultBattleBus, const UObject*);
    DEFINE_PROP(SafeZoneIndicator, AActor*);
    DEFINE_PROP(StructuralSupportSystem, UObject*);
    DEFINE_PROP(bPlaylistDataIsLoaded, bool);
    DEFINE_PROP(VolumeManager, AFortVolumeManager*);
    DEFINE_PROP(CreativePortalManager, AFortCreativePortalManager*);
    DEFINE_PROP(MissionManager, AActor*);
    DEFINE_BITFIELD_PROP(bInSpawningStartup);

    DEFINE_FUNC(OnRep_CurrentPlaylistInfo, void);
    DEFINE_FUNC(OnRep_CurrentPlaylistData, void);
    DEFINE_FUNC(OnRep_CurrentPlaylistId, void);
    DEFINE_FUNC(OnRep_AdditionalPlaylistLevelsStreamed, void);
    DEFINE_FUNC(IsRespawningAllowed, bool);
    DEFINE_FUNC(OnRep_WinningTeam, void);
    DEFINE_FUNC(OnRep_WinningPlayerState, void);
    DEFINE_FUNC(OnRep_GamePhase, void);
    DEFINE_FUNC(OnRep_PlayersLeft, void);
    DEFINE_FUNC(OnRep_SafeZoneIndicator, void);
    DEFINE_FUNC(OnRep_MissionManager, void);
};