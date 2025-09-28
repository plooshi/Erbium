#include "pch.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortGameModeAthena.h"


uint64_t FindGetPlayerViewPoint()
{
	uint64 ftspAddr = 0;
	auto ftspRef = Memcury::Scanner::FindStringRef(L"%s failed to spawn a pawn", true, 0, VersionInfo.FortniteVersion >= 19).Get();

	for (int i = 0; i < 1000; i++)
	{
		if (*(uint8_t*)(ftspRef - i) == 0x40 && *(uint8_t*)(ftspRef - i + 1) == 0x53)
		{
			ftspAddr = ftspRef - i;
			break;
		}
		else if (*(uint8_t*)(ftspRef - i) == 0x48 && *(uint8_t*)(ftspRef - i + 1) == 0x89 && *(uint8_t*)(ftspRef - i + 2) == 0x5C)
		{
			ftspAddr = ftspRef - i;
			break;
		}
	}

	if (!ftspAddr)
		return 0;

	auto PCVft = AFortPlayerControllerAthena::GetDefaultObj()->Vft;
	int ftspIdx = 0;

	for (int i = 0; i < 500; i++)
	{
		if (PCVft[i] == (void*)ftspAddr)
		{
			ftspIdx = i;
			break;
		}
	}

	if (ftspIdx == 0)
		return 0;

	return __int64(PCVft[ftspIdx - 1]);
}

void AFortPlayerControllerAthena::GetPlayerViewPoint(AFortPlayerControllerAthena* PlayerController, FVector& Loc, FRotator& Rot)
{
	static auto SFName = UKismetStringLibrary::Conv_StringToName(FString(L"Spectating"));
	if (PlayerController->StateName == SFName)
	{
		Loc = PlayerController->LastSpectatorSyncLocation;
		Rot = PlayerController->LastSpectatorSyncRotation;
	}
	else if (PlayerController->GetViewTarget())
	{
		Loc = PlayerController->GetViewTarget()->K2_GetActorLocation();
		Rot = PlayerController->GetControlRotation();
	}
	else return GetPlayerViewPointOG(PlayerController, Loc, Rot);
}

void AFortPlayerControllerAthena::ServerAcknowledgePossession(UObject* Context, FFrame& Stack)
{
	AActor* Pawn;
	Stack.StepCompiledIn(&Pawn);
	Stack.IncrementCode();
	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	PlayerController->AcknowledgedPawn = Pawn;
}

void AFortPlayerControllerAthena::ServerAttemptAircraftJump(UObject* Context, FFrame& Stack)
{
	FRotator Rotation;
	Stack.StepCompiledIn(&Rotation);
	Stack.IncrementCode();

	AFortPlayerControllerAthena* PlayerController = nullptr;
	auto GameMode = (AFortGameModeAthena*) UWorld::GetWorld()->AuthorityGameMode;

	static auto ControllerCompClass = FindClass("FortControllerComponent_Aircraft");
	if (Context->IsA(ControllerCompClass))
		PlayerController = (AFortPlayerControllerAthena*)((UActorComponent*)Context)->GetOwner();
	else
		PlayerController = (AFortPlayerControllerAthena*)Context;

	GameMode->RestartPlayer(PlayerController);
	PlayerController->ClientSetRotation(Rotation, true);

	if (PlayerController->MyFortPawn)
	{
		PlayerController->MyFortPawn->BeginSkydiving(true);
		PlayerController->MyFortPawn->SetHealth(100.f);
	}
}

void AFortPlayerControllerAthena::ServerExecuteInventoryItem(UObject* Context, FFrame& Stack)
{
	FGuid ItemGuid;
	Stack.StepCompiledIn(&ItemGuid);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)Context;
	if (!PlayerController)
		return;

	auto entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
		return entry.ItemGuid == ItemGuid;
	}, FFortItemEntry::Size());

	if (!entry || !PlayerController->MyFortPawn) 
		return;

	UFortItemDefinition* ItemDefinition = (UFortItemDefinition*) entry->ItemDefinition;
	PlayerController->MyFortPawn->EquipWeaponDefinition(ItemDefinition, ItemGuid, entry->HasTrackerGuid() ? entry->TrackerGuid : FGuid(), false);
}

void AFortPlayerControllerAthena::Hook()
{
	auto DefaultFortPC = DefaultObjImpl("FortPlayerController");

	Utils::Hook(FindGetPlayerViewPoint(), GetPlayerViewPoint, GetPlayerViewPointOG);
	// they only stripped it on athena for some reason
	auto ServerAcknowledgePossessionIdx = GetDefaultObj()->GetFunction("ServerAcknowledgePossession")->GetVTableIndex();
	Utils::Hook<AFortPlayerControllerAthena>(ServerAcknowledgePossessionIdx, DefaultFortPC->Vft[ServerAcknowledgePossessionIdx]);

	auto ServerAttemptAircraftJumpPC = GetDefaultObj()->GetFunction("ServerAttemptAircraftJump");
	if (!ServerAttemptAircraftJumpPC)
		Utils::ExecHook(DefaultObjImpl("FortControllerComponent_Aircraft")->GetFunction("ServerAttemptAircraftJump"), ServerAttemptAircraftJump);
	else
		Utils::ExecHook(ServerAttemptAircraftJumpPC, ServerAttemptAircraftJump);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerAcknowledgePossession"), ServerAcknowledgePossession);
	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerExecuteInventoryItem"), ServerExecuteInventoryItem);

	// same as serveracknowledgepossession
	auto ServerReturnToMainMenuIdx = GetDefaultObj()->GetFunction("ServerReturnToMainMenu")->GetVTableIndex();
	Utils::Hook<AFortPlayerControllerAthena>(ServerReturnToMainMenuIdx, DefaultFortPC->Vft[ServerReturnToMainMenuIdx]);
}