#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"
#include "FortPlayerPawnAthena.h"
#include "FortInventory.h"
#include "FortPlayerStateAthena.h"


class UAthenaPickaxeItemDefinition : public UObject
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

    DEFINE_FUNC(GetViewTarget, AActor*);
    DEFINE_FUNC(GetControlRotation, FRotator);
    DEFINE_FUNC(ClientSetRotation, void);
    DEFINE_FUNC(ClientReportDamagedResourceBuilding, void);

    static void ServerAcknowledgePossession(UObject*, FFrame&);
    DefHookOg(void, GetPlayerViewPoint, AFortPlayerControllerAthena*, FVector&, FRotator&);
    static void ServerAttemptAircraftJump(UObject*, FFrame&);
    static void ServerExecuteInventoryItem(UObject*, FFrame&);
    static void ServerExecuteInventoryWeapon(UObject*, FFrame&);
    static void ServerCreateBuildingActor(UObject*, FFrame&);
    static void ServerBeginEditingBuildingActor(UObject*, FFrame&);
    static void ServerEditBuildingActor(UObject*, FFrame&);
    static void ServerEndEditingBuildingActor(UObject*, FFrame&);
    static void ServerRepairBuildingActor(UObject*, FFrame&);
    

    InitHooks;
};