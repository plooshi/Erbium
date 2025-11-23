#pragma once
#include "../../pch.h"
#include "FortPlayerPawnAthena.h"
#include "FortInventory.h"
#include "FortPlayerStateAthena.h"
#include "GameplayTagContainer.h"
#include "FortVolume.h"


class UAthenaPickaxeItemDefinition : public UFortItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UAthenaPickaxeItemDefinition);

    DEFINE_PROP(WeaponDefinition, UFortItemDefinition*);
};

struct FFortAthenaLoadout
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortAthenaLoadout);

    DEFINE_STRUCT_PROP(Pickaxe, UAthenaPickaxeItemDefinition*);
};

struct FFortPlayerDeathReport
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortPlayerDeathReport);

    DEFINE_STRUCT_PROP(KillerPlayerState, AFortPlayerStateAthena*);
    DEFINE_STRUCT_NEWOBJ_PROP(KillerPawn, AFortPlayerPawnAthena);
    DEFINE_STRUCT_PROP(Tags, FGameplayTagContainer);
    DEFINE_STRUCT_NEWOBJ_PROP(DamageCauser, AActor);
};

class UAthenaDanceItemDefinition : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UAthenaDanceItemDefinition);

    DEFINE_BITFIELD_PROP(bMovingEmote);
    DEFINE_PROP(WalkForwardSpeed, float);
    DEFINE_BITFIELD_PROP(bMoveForwardOnly);
    DEFINE_BITFIELD_PROP(bMoveFollowingOnly);
};

class UFortControllerComponent_Aircraft : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortControllerComponent_Aircraft);

    DEFINE_FUNC(ServerAttemptAircraftJump, void);
    DEFINE_FUNC(KickFromAircraft, void);
};

struct FQuickBarSlot
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FQuickBarSlot);

    DEFINE_STRUCT_PROP(Items, TArray<FGuid>);
    DEFINE_STRUCT_PROP(bEnabled, bool);
};

struct FQuickBar
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FQuickBar);

    DEFINE_STRUCT_PROP(Slots, TArray<FQuickBarSlot>);
};

class AFortQuickBars : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortQuickBars);

    DEFINE_PROP(PrimaryQuickBar, FQuickBar);
    DEFINE_PROP(SecondaryQuickBar, FQuickBar);

    DEFINE_FUNC(ServerActivateSlotInternal, void);
    DEFINE_FUNC(ServerAddItemInternal, void);
    DEFINE_FUNC(ServerRemoveItemInternal, void);
    DEFINE_FUNC(EmptySlot, void);
    DEFINE_FUNC(OnRep_PrimaryQuickBar, void);
    DEFINE_FUNC(OnRep_SecondaryQuickBar, void);
};

class UFortControllerComponent_Interaction : public UActorComponent
{
public:
    UCLASS_COMMON_MEMBERS(UFortControllerComponent_Interaction);

    DEFINE_FUNC(ServerAttemptInteract, void);
};

// todo: seperate this
class UFortPlayerControllerAthenaXPComponent : public UActorComponent
{
public:
    UCLASS_COMMON_MEMBERS(UFortPlayerControllerAthenaXPComponent);

    DEFINE_PROP(CurrentLevel, int32);
    DEFINE_PROP(bRegisteredWithQuestManager, bool);

    DEFINE_FUNC(OnRep_bRegisteredWithQuestManager, void);
};

class AFortBroadcastRemoteClientInfo : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortBroadcastRemoteClientInfo);

    DEFINE_PROP(RemoteBuildableClass, TSubclassOf<AActor>);
    DEFINE_PROP(bActive, bool);

    DEFINE_FUNC(OnRep_bActive, void);
};

struct FBuildingClassData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FBuildingClassData);

    TSubclassOf<AActor> BuildingClass;
    int PreviousBuildingLevel;
    int UpgradeLevel;
};

inline const UCurveTable* GameData = nullptr;
class AFortPlayerControllerAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPlayerControllerAthena);

    DEFINE_PROP(AcknowledgedPawn, AActor*);
    DEFINE_PROP(StateName, FName);
    DEFINE_PROP(LastSpectatorSyncLocation, FVector);
    DEFINE_PROP(LastSpectatorSyncRotation, FRotator);
    DEFINE_PROP(PlayerState, AFortPlayerStateAthena*);
    DEFINE_PROP(MyFortPawn, AFortPlayerPawnAthena*);
    DEFINE_PROP(Pawn, AFortPlayerPawnAthena*);
    DEFINE_PROP(WorldInventory, AFortInventory*);
    DEFINE_PROP(CosmeticLoadoutPC, FFortAthenaLoadout);
    DEFINE_PROP(CustomizationLoadout, FFortAthenaLoadout);
    DEFINE_BITFIELD_PROP(bBuildFree);
    DEFINE_BITFIELD_PROP(bInfiniteAmmo);
    DEFINE_PROP(SwappingItemDefinition, FFortItemEntry*); // scuffness
    DEFINE_PROP(QuickBars, AFortQuickBars*);
    DEFINE_PROP(XPComponent, UFortPlayerControllerAthenaXPComponent*);
    DEFINE_PROP(CheatManager, UObject*);
    DEFINE_PROP(CheatClass, TSubclassOf<UObject>);
    DEFINE_PROP(WorldInventoryClass, TSubclassOf<AFortInventory>);
    DEFINE_PROP(bHasInitializedWorldInventory, bool);
    DEFINE_PROP(ActiveToyInstances, TArray<AActor*>);
    DEFINE_PROP(AppliedInGameModifierAbilitySetHandles, TMap<FGuid, void*>);
    DEFINE_PROP(BroadcastRemoteClientInfo, AFortBroadcastRemoteClientInfo*);
    DEFINE_BITFIELD_PROP(bTryPickupSwap);
    DEFINE_PROP(bEnableBroadcastRemoteClientInfo, bool);
    DEFINE_BITFIELD_PROP(bReadyToStartMatch);
    DEFINE_BITFIELD_PROP(bHoldingObject);
    DEFINE_PROP(StrongMyHero, UObject*);
    DEFINE_PROP(OwnedPortal, AActor*);
    DEFINE_PROP(CreativePlotLinkedVolume, AFortVolume*);
    DEFINE_PROP(CurrentResourceType, EFortResourceType);
    DEFINE_PROP(WarmupPlayerStart, AActor*);

    DEFINE_FUNC(GetViewTarget, AActor*);
    DEFINE_FUNC(GetControlRotation, FRotator);
    DEFINE_FUNC(SetControlRotation, void);
    DEFINE_FUNC(ClientSetRotation, void);
    DEFINE_FUNC(ClientReportDamagedResourceBuilding, void);
    DEFINE_FUNC(PlayWinEffects, void);
    DEFINE_FUNC(ClientNotifyWon, void);
    DEFINE_FUNC(ClientNotifyTeamWon, void);
    DEFINE_FUNC(ClientMessage, void);
    DEFINE_FUNC(ClientGotoState, void);
    DEFINE_FUNC(IsInAircraft, bool);
    DEFINE_FUNC(ServerSetTeam, void);
    DEFINE_FUNC(GetAircraftComponent, UFortControllerComponent_Aircraft*);
    DEFINE_FUNC(ServerAttemptAircraftJump, void);
    DEFINE_FUNC(Possess, void);
    DEFINE_FUNC(RespawnPlayerAfterDeath, void);
    DEFINE_FUNC(ServerAttemptInteract, void);
    DEFINE_FUNC(ServerExecuteInventoryItem, void);
    DEFINE_FUNC(ClientEquipItem, void);
    DEFINE_FUNC(OnRep_PlayerState, void);
    DEFINE_FUNC(ServerChangeName, void);
    DEFINE_FUNC(ClientIgnoreMoveInput, void);
    DEFINE_FUNC(ClientIgnoreLookInput, void);
    DEFINE_FUNC(GetActorEyesViewPoint, void);
    DEFINE_FUNC(ClientActivateSlot, void);
    DEFINE_FUNC(ServerReturnToMainMenu, void);
    DEFINE_FUNC(OnRep_CreativePlotLinkedVolume, void);
    DEFINE_FUNC(ClientRemoveItemAbilitySet, void);


    static void ServerAcknowledgePossession(UObject*, FFrame&);
    DefHookOg(void, GetPlayerViewPoint, AFortPlayerControllerAthena*, FVector&, FRotator&);
    DefHookOg(void, ServerAttemptAircraftJump_, UObject*, FFrame&);
    static void ServerExecuteInventoryItem_(UObject*, FFrame&);
    static void ServerExecuteInventoryWeapon(UObject*, FFrame&);
    static void ServerCreateBuildingActor(UObject*, FFrame&);
    static void ServerBeginEditingBuildingActor(UObject*, FFrame&);
    static void ServerEditBuildingActor(UObject*, FFrame&);
    static void ServerEndEditingBuildingActor(UObject*, FFrame&);
    static void ServerRepairBuildingActor(UObject*, FFrame&);
    static void ServerAttemptInventoryDrop(UObject*, FFrame&);
    static void ServerPlayEmoteItem(UObject*, FFrame&);
    static void ServerClientIsReadyToRespawn(UObject*, FFrame&);
    static void ServerCheat(UObject*, FFrame&);
    DefHookOg(void, ClientOnPawnDied, AFortPlayerControllerAthena*, FFortPlayerDeathReport&);
    DefUHookOg(ServerAttemptInteract_);
    void InternalPickup(FFortItemEntry*);
    static void ServerDropAllItems(UObject*, FFrame&);
    static void SpawnToyInstance(UObject*, FFrame&, AActor**);
    DefHookOg(void, EnterAircraft, UObject*, AActor*);
    static void ServerTeleportToPlaygroundLobbyIsland(UObject*, FFrame&);
    static void ServerCraftSchematic(UObject*, FFrame&);
    static void ServerGiveCreativeItem(UObject*, FFrame&);


    InitPostLoadHooks;
};