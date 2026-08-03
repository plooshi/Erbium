#include "pch.h"
#include "../Public/FortVehicleSeatWeaponComponent.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortWeapon.h"

void UFortVehicleSeatWeaponComponent::EquipVehicleWeapon(UFortVehicleSeatWeaponComponent* _this, AFortPlayerPawnAthena* FortPawn, FWeaponSeatDefinition* WeaponSeatDefinition, int ItemLevel)
{
    printf(__FUNCTION__ "\n");

    auto PlayerController = (AFortPlayerControllerAthena*)FortPawn->Controller;

    auto VehicleWeapon = WeaponSeatDefinition->HasVehicleWeaponOverride() && WeaponSeatDefinition->VehicleWeaponOverride ? WeaponSeatDefinition->VehicleWeaponOverride : WeaponSeatDefinition->VehicleWeapon;

    auto VehicleItem = PlayerController->WorldInventory->GiveItem(VehicleWeapon, 1, AFortInventory::GetStats(VehicleWeapon)->ClipSize);

    if (!VehicleItem)
        return;

    auto Weapon = (AFortWeapon*)PlayerController->MyFortPawn->EquipWeaponDefinition(VehicleItem->ItemEntry.ItemDefinition, VehicleItem->ItemEntry.ItemGuid,
                                                                                    FFortItemEntry::HasTrackerGuid() ? VehicleItem->ItemEntry.TrackerGuid : FGuid(), false);
    Weapon->ForceNetUpdate();

    if (VersionInfo.FortniteVersion >= 21.20) // guess
    {
        auto& VehicleGrantedWeaponItem = *(TWeakObjectPtr<UFortWorldItem>*)(uint64_t(&WeaponSeatDefinition->LastEquippedVehicleWeapon) + 0xC);
        auto& VehicleGrantedWeapon = *(TWeakObjectPtr<AFortWeapon>*)(uint64_t(&WeaponSeatDefinition->LastEquippedVehicleWeapon) + 0x14);

        VehicleGrantedWeaponItem = VehicleItem;
        VehicleGrantedWeapon = Weapon;
    }
    else
    {
        auto Inc = VersionInfo.FortniteVersion >= 20 ? 4 : 0;

        auto& VehicleGrantedWeaponItem = *(UFortWorldItem**)(uint64_t(&WeaponSeatDefinition->LastEquippedVehicleWeapon) + 0x8 + Inc);
        auto& VehicleGrantedWeapon = *(AFortWeapon**)(uint64_t(&WeaponSeatDefinition->LastEquippedVehicleWeapon) + 0x10 + Inc);

        VehicleGrantedWeaponItem = VehicleItem;
        VehicleGrantedWeapon = Weapon;
    }

    _this->CachedWeapon = Weapon;
    _this->CachedWeaponDef = Weapon->WeaponData;
    _this->bWeaponEquipped = true;

    auto Vehicle = _this->GetVehicle();

    if (Weapon)
    {
        auto RepWeaponInfo = (FMountedWeaponInfoRepped*)malloc(FMountedWeaponInfoRepped::Size());

        if (RepWeaponInfo->HasHostVehicleCached())
        {
            RepWeaponInfo->HostVehicleCached.ObjectPointer = Vehicle;
            RepWeaponInfo->HostVehicleCached.InterfacePointer = Vehicle->GetInterface(IFortVehicleInterface::StaticClass());
        }
        else
            RepWeaponInfo->HostVehicleCachedActor = Vehicle;

        RepWeaponInfo->HostVehicleSeatIndexCached = WeaponSeatDefinition->SeatIndex;

        static auto DualClass = FindClass("FortWeaponRangedDualForVehicle");

        if (Weapon->IsA(DualClass))
        {
            static auto MountedWeaponInfoReppedOff = Weapon->GetOffset("MountedWeaponInfoRepped");
            static auto OnRep_MountedWeaponInfoRepped = Weapon->GetFunction("OnRep_MountedWeaponInfoRepped");
            *(FMountedWeaponInfoRepped*)(__int64(Weapon) + MountedWeaponInfoReppedOff) = *RepWeaponInfo;
            Weapon->Call(OnRep_MountedWeaponInfoRepped);
        }
        else
        {
            static auto MountedWeaponInfoReppedOff = Weapon->GetOffset("MountedWeaponInfoRepped");
            static auto OnRep_MountedWeaponInfoRepped = Weapon->GetFunction("OnRep_MountedWeaponInfoRepped");
            *(FMountedWeaponInfoRepped*)(__int64(Weapon) + MountedWeaponInfoReppedOff) = *RepWeaponInfo;
            Weapon->Call(OnRep_MountedWeaponInfoRepped);
        }

        free(RepWeaponInfo);
    }

    WeaponSeatDefinition->LastEquippedVehicleWeapon = VehicleWeapon;
}

void UFortVehicleSeatWeaponComponent::EquipVehicleWeapon_(UFortVehicleSeatWeaponComponent* _this, AFortPlayerPawnAthena* FortPawn, FWeaponSeatDefinition* WeaponSeatDefinition, int ItemLevel)
{
    EquipVehicleWeapon(_this, FortPawn, WeaponSeatDefinition, ItemLevel);

    return EquipVehicleWeapon_OG(_this, FortPawn, WeaponSeatDefinition, ItemLevel);
}

uint32_t GetMountedWeaponOperatorSeatIndexIdx = 0;
uint32_t GetPlayerSlotsIdx = 0;

void UFortVehicleSeatWeaponComponent::OnPawnEnterSeat(UFortVehicleSeatWeaponComponent* _this, IFortVehicleInterface* VehicleOwnerInterface, int SeatIndex, bool bSeatSwitch)
{
    auto& GetMountedWeaponOperatorSeatIndex = (int (*&)(IFortVehicleInterface*))VehicleOwnerInterface->Vft[GetMountedWeaponOperatorSeatIndexIdx];
    auto& GetPlayerSlots = (TArray<FAthenaCarPlayerSlot>& (*&)(IFortVehicleInterface*))VehicleOwnerInterface->Vft[GetPlayerSlotsIdx];

    for (int i = 0; i < _this->WeaponSeatDefinitions.Num(); i++)
    {
        auto& SeatDefinition = _this->WeaponSeatDefinitions.Get(i, FWeaponSeatDefinition::Size());

        if (SeatIndex != SeatDefinition.SeatIndex)
            continue;

        int OperatorIdx = GetMountedWeaponOperatorSeatIndex(VehicleOwnerInterface);

        if (OperatorIdx == -1 || OperatorIdx == SeatIndex)
            _this->ActiveSeatIdx = SeatIndex;

        auto PlayerSlots = GetPlayerSlots(VehicleOwnerInterface);

        if (!PlayerSlots.IsValidIndex(i))
            continue;

        auto& PlayerSlot = PlayerSlots.Get(i, FAthenaCarPlayerSlot::Size());

        auto Player = PlayerSlot.Player;

        if (Player)
            EquipVehicleWeapon(_this, Player, &SeatDefinition, 0);
    }
}

// for some reason this just doesnt get called
// need to make this get called
// or find the proper way to do it
void UFortVehicleSeatWeaponComponent::UnEquipVehicleWeapon(UFortVehicleSeatWeaponComponent* _this, AFortPlayerPawnAthena* FortPawn, FWeaponSeatDefinition* WeaponSeatDefinition, bool bRequiresEquipValidWeapon)
{
    printf(__FUNCTION__ "\n");

    return UnEquipVehicleWeaponOG(_this, FortPawn, WeaponSeatDefinition, bRequiresEquipValidWeapon);
}

void UFortVehicleSeatWeaponComponent::PostLoadHook()
{
    if (!UFortVehicleSeatWeaponComponent::GetDefaultObj())
        return;

    if (VersionInfo.FortniteVersion < 20)
    {
        // old path: EquipVehicleWeapon func is stripped
        // we have to reimplement onpawnenterseat to call EquipVehicleWeapon
        auto OnPawnEnterSeatFunc = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC ? 48 8B 99 ? ? ? ? 41 8B F0").Get();

        if (!OnPawnEnterSeatFunc)
            OnPawnEnterSeatFunc = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 83 EC ? 48 63 A9 ? ? ? ? 41 8B F0").Get();

        GetMountedWeaponOperatorSeatIndexIdx = *(uint32_t*)(Memcury::Scanner(OnPawnEnterSeatFunc).ScanFor({ 0xFF, 0x90 }).Get() + 2) / 8;
        GetPlayerSlotsIdx = *(uint32_t*)(Memcury::Scanner(OnPawnEnterSeatFunc).ScanFor({ 0xFF, 0x90 }, true, 1).Get() + 2) / 8;

        Hooking::Hook(OnPawnEnterSeatFunc, OnPawnEnterSeat);

        //Hooking::Hook(FindUnEquipVehicleWeapon(), UnEquipVehicleWeapon, UnEquipVehicleWeaponOG);
    }
    else
    {
        // new path: function exists, but w/o logic
        // function is left around due to replicated target event
        auto EquipVehicleWeaponFunc = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC ? 41 80 78 ? ? 48 8B DA 48 8B F9").Get();

        Hooking::Hook(EquipVehicleWeaponFunc, EquipVehicleWeapon_, EquipVehicleWeapon_OG);
        
        // new path: SetVehicleSeatWeaponOverride doesnt have a string, so we have to find it based on sigs
        // the string inside UnequipVehicleWeapon is in a function chunk
        auto UnequipVehicleWeaponFunc = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 45 33 F6 45 8A E9").Get();

        if (!UnequipVehicleWeaponFunc)
            UnequipVehicleWeaponFunc = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 44 88 48 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 48 8B 81").Get();

        if (!UnequipVehicleWeaponFunc)
            UnequipVehicleWeaponFunc = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 44 88 48 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 4D 8B E8").Get();

        //Hooking::Hook(UnequipVehicleWeaponFunc, UnEquipVehicleWeapon, UnEquipVehicleWeaponOG);
    }
}