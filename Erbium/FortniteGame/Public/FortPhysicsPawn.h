#pragma once
#include "../../pch.h"
#include "FortInventory.h"

class AFortPhysicsPawn : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPhysicsPawn);

    static void ServerMove(UObject*, FFrame&);

    InitHooks;
};

struct FWeaponSeatDefinition
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FWeaponSeatDefinition);

    DEFINE_STRUCT_PROP(SeatIndex, int32);
    DEFINE_STRUCT_PROP(VehicleWeapon, UFortWeaponItemDefinition*);
};

class UFortVehicleSeatWeaponComponent : public UActorComponent
{
public:
    UCLASS_COMMON_MEMBERS(UFortVehicleSeatWeaponComponent);

    DEFINE_PROP(WeaponSeatDefinitions, TArray<FWeaponSeatDefinition>);
};

class AFortAthenaVehicle : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAthenaVehicle);

    DEFINE_FUNC(FindSeatIndex, int32);
};

struct FNetTowhookAttachState
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FNetTowhookAttachState);

    DEFINE_STRUCT_PROP(Component, UActorComponent*);
    DEFINE_STRUCT_PROP(LocalLocation, FVector);
    DEFINE_STRUCT_PROP(LocalNormal, FVector);
};

class AFortOctopusVehicle : public AFortPhysicsPawn
{
public:
    UCLASS_COMMON_MEMBERS(AFortOctopusVehicle);

    DEFINE_PROP(NetTowhookAimDir, FVector);
    DEFINE_PROP(ReplicatedAttachState, FNetTowhookAttachState);

    DEFINE_FUNC(OnRep_NetTowhookAimDir, void);
    DEFINE_FUNC(OnRep_ReplicatedAttachState, void);

    static void ServerUpdateTowhook(UObject*, FFrame&);
};

class AFortSpaghettiVehicle : public AFortPhysicsPawn
{
public:
    UCLASS_COMMON_MEMBERS(AFortSpaghettiVehicle);

    DEFINE_PROP(NetTowhookAimDir, FVector);

    DEFINE_FUNC(OnRep_NetTowhookAimDir, void);

    static void ServerUpdateTowhook(UObject*, FFrame&);
};

class AFortDagwoodVehicle : public AFortAthenaVehicle
{
public:
    UCLASS_COMMON_MEMBERS(AFortDagwoodVehicle);

    DEFINE_FUNC(SetFuel, float);
};