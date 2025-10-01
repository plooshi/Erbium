#include "pch.h"
#include "../Public/FortPlayerPawnAthena.h"
#include "../Public/FortInventory.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortWeapon.h"

struct _Pad_0xC
{
	uint8_t Padding[0xC];
};

struct _Pad_0x18
{
	uint8_t Padding[0x18];
};

struct FFortPickupRequestInfo final
{
public:
	struct FGuid                                  SwapWithItem;                                      // 0x0000(0x0010)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                         FlyTime;                                           // 0x0010(0x0004)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	struct _Pad_0xC                               Direction;                                         // 0x0014(0x000C)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8                                         bPlayPickupSound : 1;                              // 0x0020(0x0001)(BitIndex: 0x00, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bIsAutoPickup : 1;                                 // 0x0020(0x0001)(BitIndex: 0x01, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bUseRequestedSwap : 1;                             // 0x0020(0x0001)(BitIndex: 0x02, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bTrySwapWithWeapon : 1;                            // 0x0020(0x0001)(BitIndex: 0x03, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         Pad_21[0x3];                                       // 0x0021(0x0003)(Fixing Struct Size After Last Property [ Dumper-7 ])
};

struct FFortPickupRequestInfoNew final
{
public:
	struct FGuid                                  SwapWithItem;                                      // 0x0000(0x0010)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                         FlyTime;                                           // 0x0010(0x0004)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	struct _Pad_0x18                              Direction;                                        // 0x0014(0x000C)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8                                         bPlayPickupSound : 1;                              // 0x0020(0x0001)(BitIndex: 0x00, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bIsAutoPickup : 1;                                 // 0x0020(0x0001)(BitIndex: 0x01, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bUseRequestedSwap : 1;                             // 0x0020(0x0001)(BitIndex: 0x02, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bTrySwapWithWeapon : 1;                            // 0x0020(0x0001)(BitIndex: 0x03, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         Pad_21[0x3];                                       // 0x0021(0x0003)(Fixing Struct Size After Last Property [ Dumper-7 ])
};


void AFortPlayerPawnAthena::ServerHandlePickup_(UObject* Context, FFrame& Stack)
{
	AFortPickupAthena* Pickup;
	float InFlyTime;
	FVector InStartDirection;
	bool bPlayPickupSound;
	Stack.StepCompiledIn(&Pickup);
	Stack.StepCompiledIn(&InFlyTime);
	Stack.StepCompiledIn(&InStartDirection);
	Stack.StepCompiledIn(&bPlayPickupSound);
	Stack.IncrementCode();
	auto Pawn = (AFortPlayerPawnAthena*)Context;
	if (!Pawn || !Pickup || Pickup->bPickedUp)
		return;

	Pawn->IncomingPickups.Add(Pickup);

	Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
	Pickup->PickupLocationData.FlyTime = 0.4f;
	Pickup->PickupLocationData.ItemOwner = Pawn;
	Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
	Pickup->PickupLocationData.PickupTarget = Pawn;
	//Pickup->PickupLocationData.StartDirection = (FVector_NetQuantizeNormal)InStartDirection;
	Pickup->OnRep_PickupLocationData();

	Pickup->bPickedUp = true;
	Pickup->OnRep_bPickedUp();
}

void AFortPlayerPawnAthena::ServerHandlePickupInfo(UObject* Context, FFrame& Stack)
{
	AFortPickupAthena* Pickup;
	FFortPickupRequestInfo Params;
	Stack.StepCompiledIn(&Pickup);
	Stack.StepCompiledIn(&Params);
	Stack.IncrementCode();
	auto Pawn = (AFortPlayerPawnAthena*)Context;

	if (!Pawn || !Pickup || Pickup->bPickedUp)
		return;

	if ((Params.bTrySwapWithWeapon || Params.bUseRequestedSwap) && Pawn->CurrentWeapon && AFortInventory::IsPrimaryQuickbar(((AFortWeapon*)Pawn->CurrentWeapon)->WeaponData) && AFortInventory::IsPrimaryQuickbar(Pickup->PrimaryPickupItemEntry.ItemDefinition))
	{
		auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
		auto SwapEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{ return entry.ItemGuid == Params.SwapWithItem; }, FFortItemEntry::Size());
		PlayerController->SwappingItemDefinition = SwapEntry; // proper af
	}
	Pawn->IncomingPickups.Add(Pickup);

	Pickup->PickupLocationData.bPlayPickupSound = Params.bPlayPickupSound;
	Pickup->PickupLocationData.FlyTime = 0.4f;
	Pickup->PickupLocationData.ItemOwner = Pawn;
	Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
	Pickup->PickupLocationData.PickupTarget = Pawn;
	//Pickup->PickupLocationData.StartDirection = Params.Direction.QuantizeNormal();
	Pickup->OnRep_PickupLocationData();

	Pickup->bPickedUp = true;
	Pickup->OnRep_bPickedUp();
}


void InternalPickup(AFortPlayerControllerAthena* PlayerController, FFortItemEntry* PickupEntry)
{
	auto MaxStack = (int32)PickupEntry->ItemDefinition->GetMaxStackSize();
	int ItemCount = 0;

	for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
	{
		auto& Item = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Get(i, FFortItemEntry::Size());

		if (AFortInventory::IsPrimaryQuickbar(Item.ItemDefinition))
			ItemCount += Item.ItemDefinition->HasNumberOfSlotsToTake() ? Item.ItemDefinition->NumberOfSlotsToTake : 2;
	}

	auto GiveOrSwap = [&]() 
	{
		if (ItemCount == 5 && AFortInventory::IsPrimaryQuickbar(PickupEntry->ItemDefinition)) {
			if (AFortInventory::IsPrimaryQuickbar(((AFortWeapon*)PlayerController->MyFortPawn->CurrentWeapon)->WeaponData)) 
			{
				auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([PlayerController](FFortItemEntry& entry)
					{ return entry.ItemGuid == ((AFortWeapon*)PlayerController->MyFortPawn->CurrentWeapon)->ItemEntryGuid; }, FFortItemEntry::Size());

				AFortInventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), *itemEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), PlayerController->MyFortPawn);
				PlayerController->WorldInventory->Remove(((AFortWeapon*)PlayerController->MyFortPawn->CurrentWeapon)->ItemEntryGuid);
				PlayerController->WorldInventory->GiveItem(*PickupEntry, PickupEntry->Count, true);
			}
			else
				AFortInventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), (FFortItemEntry&)PickupEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), PlayerController->MyFortPawn);
		}
		else
			PlayerController->WorldInventory->GiveItem(*PickupEntry, PickupEntry->Count, true);
	};

	auto GiveOrSwapStack = [&](int32 OriginalCount) 
	{
		if (PickupEntry->ItemDefinition->bAllowMultipleStacks && ItemCount < 5)
			PlayerController->WorldInventory->GiveItem(*PickupEntry, OriginalCount - MaxStack, true);
		else
			AFortInventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), (FFortItemEntry&)PickupEntry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), PlayerController->MyFortPawn, OriginalCount - MaxStack);
	};

	if (MaxStack > 1)
	{
		auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([PickupEntry, MaxStack](FFortItemEntry& entry)
			{ return entry.ItemDefinition == PickupEntry->ItemDefinition && entry.Count < MaxStack; }, FFortItemEntry::Size());

		if (itemEntry) 
		{
			if ((itemEntry->Count += PickupEntry->Count) > MaxStack) 
			{
				auto OriginalCount = itemEntry->Count;
				itemEntry->Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}

			PlayerController->WorldInventory->UpdateEntry(*itemEntry);
		}
		else 
		{
			if (PickupEntry->Count > MaxStack) 
			{
				auto OriginalCount = PickupEntry->Count;
				PickupEntry->Count = MaxStack;

				GiveOrSwapStack(OriginalCount);
			}

			GiveOrSwap();
		}
	}
	else
		GiveOrSwap();
}



bool AFortPlayerPawnAthena::FinishedTargetSpline(void* _Pickup) {
	auto Pickup = (AFortPickupAthena*)_Pickup;

	auto Pawn = (AFortPlayerPawnAthena*)Pickup->PickupLocationData.PickupTarget;
	if (!Pawn)
		return FinishedTargetSplineOG(Pickup);

	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PlayerController)
		return FinishedTargetSplineOG(Pickup);

	if (auto entry = (FFortItemEntry*)PlayerController->SwappingItemDefinition)
	{
		AFortInventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), *entry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), PlayerController->MyFortPawn);
		// SwapEntry(PC, *entry, Pickup->PrimaryPickupItemEntry);
		PlayerController->WorldInventory->Remove(entry->ItemGuid);
		PlayerController->WorldInventory->GiveItem(Pickup->PrimaryPickupItemEntry);
		PlayerController->SwappingItemDefinition = nullptr;
	}
	else
		InternalPickup(PlayerController, &Pickup->PrimaryPickupItemEntry);

	return FinishedTargetSplineOG(Pickup);
}

void AFortPlayerPawnAthena::Hook()
{
	auto ServerHandlePickupInfoFn = GetDefaultObj()->GetFunction("ServerHandlePickupInfo");

	if (ServerHandlePickupInfoFn)
		Utils::ExecHook(ServerHandlePickupInfoFn, ServerHandlePickupInfo);
	else
	{
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerHandlePickup"), ServerHandlePickup_);
	}
	
	Utils::Hook(FindFinishedTargetSpline(), FinishedTargetSpline, FinishedTargetSplineOG);
}