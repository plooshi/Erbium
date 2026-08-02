#pragma once
#include "../../pch.h"
#include "FortInventory.h"

class IFortVehicleInterface : public IInterface
{
public:
    UCLASS_COMMON_MEMBERS(IFortVehicleInterface);
};

struct FWeaponSeatDefinition
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FWeaponSeatDefinition);

    DEFINE_STRUCT_PROP(SeatIndex, int32);
    DEFINE_STRUCT_PROP(VehicleWeapon, UFortWeaponItemDefinition*);
    DEFINE_STRUCT_PROP(VehicleWeaponOverride, UFortWeaponItemDefinition*);
    DEFINE_STRUCT_PROP(LastEquippedVehicleWeapon, UFortWeaponItemDefinition*);
};

struct FAthenaCarPlayerSlot
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FAthenaCarPlayerSlot);

    DEFINE_STRUCT_NEWOBJ_PROP(Player, AFortPlayerPawnAthena);
};

struct FMountedWeaponInfoRepped
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FMountedWeaponInfoRepped);
    uint8_t Padding[0x48];

    DEFINE_STRUCT_PROP(HostVehicleCached, TScriptInterface<IFortVehicleInterface>);
    DEFINE_STRUCT_PROP(HostVehicleCachedActor, AActor*);
    DEFINE_STRUCT_PROP(HostVehicleSeatIndexCached, int32);
};


class UFortVehicleSeatWeaponComponent : public UActorComponent
{
public:
    UCLASS_COMMON_MEMBERS(UFortVehicleSeatWeaponComponent);

    DEFINE_PROP(WeaponSeatDefinitions, TArray<FWeaponSeatDefinition>);
    DEFINE_PROP(ActiveSeatIdx, int32);
    DEFINE_NEWOBJ_PROP(CachedWeapon, AActor);
    DEFINE_PROP(CachedWeaponDef, UFortWeaponItemDefinition*);
    DEFINE_PROP(bWeaponEquipped, bool);

    DEFINE_FUNC(GetVehicle, AActor*);

    static void OnPawnEnterSeat(UFortVehicleSeatWeaponComponent* _this, IFortVehicleInterface* VehicleOwnerInterface, const int SeatIndex, bool bSeatSwitch);
    static void EquipVehicleWeapon(UFortVehicleSeatWeaponComponent* _this, AFortPlayerPawnAthena* FortPawn, FWeaponSeatDefinition* WeaponSeatDefinition, int ItemLevel);
    DefHookOg(void, EquipVehicleWeapon_, UFortVehicleSeatWeaponComponent* _this, AFortPlayerPawnAthena* FortPawn, FWeaponSeatDefinition* WeaponSeatDefinition, int ItemLevel);
    DefHookOg(void, UnEquipVehicleWeapon, UFortVehicleSeatWeaponComponent* _this, AFortPlayerPawnAthena* FortPawn, FWeaponSeatDefinition* WeaponSeatDefinition, bool bRequiresEquipValidWeapon);

    InitPostLoadHooks;
};