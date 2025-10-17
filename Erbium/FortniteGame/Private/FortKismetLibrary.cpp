#include "pch.h"
#include "../Public/FortKismetLibrary.h"
#include "../Public/FortInventory.h"
#include "../Public/FortLootPackage.h"

bool bHasbPickupOnlyRelevantToOwner = false;
bool bHasbToss = false;
bool bHasbRandomRotation = false;
bool bHasbBlockedFromAutoPickup = false;
bool bHasPickupInstigatorHandle2 = false;
bool bHasSourceType = false;
bool bHasSource = false;
bool bHasOptionalOwnerPC = false;

void UFortKismetLibrary::K2_SpawnPickupInWorld(UObject* Object, FFrame& Stack, AFortPickupAthena** Ret)
{
	class UObject* WorldContextObject;
	class UFortItemDefinition* ItemDefinition;
	int32 NumberToSpawn;
	FVector Position;
	FVector Direction;
	int32 OverrideMaxStackCount;
	bool bToss = true;
	bool bRandomRotation = true;
	bool bBlockedFromAutoPickup = false;
	int32 PickupInstigatorHandle = 0;
	uint8 SourceType = 0;
	uint8 Source = 0;
	class AFortPlayerControllerAthena* OptionalOwnerPC = nullptr;
	bool bPickupOnlyRelevantToOwner = false;
	Stack.StepCompiledIn(&WorldContextObject);
	Stack.StepCompiledIn(&ItemDefinition);
	Stack.StepCompiledIn(&NumberToSpawn);
	Stack.StepCompiledIn(&Position);
	Stack.StepCompiledIn(&Direction);
	Stack.StepCompiledIn(&OverrideMaxStackCount);
	if (bHasbToss)
		Stack.StepCompiledIn(&bToss);
	if (bHasbRandomRotation)
		Stack.StepCompiledIn(&bRandomRotation);
	if (bHasbBlockedFromAutoPickup)
		Stack.StepCompiledIn(&bBlockedFromAutoPickup);
	if (bHasPickupInstigatorHandle2)
		Stack.StepCompiledIn(&PickupInstigatorHandle);
	if (bHasSourceType)
		Stack.StepCompiledIn(&SourceType);
	if (bHasSource)
		Stack.StepCompiledIn(&Source);
	if (bHasOptionalOwnerPC)
		Stack.StepCompiledIn(&OptionalOwnerPC);
	if (bHasbPickupOnlyRelevantToOwner)
		Stack.StepCompiledIn(&bPickupOnlyRelevantToOwner);
	Stack.IncrementCode();

	*Ret = AFortInventory::SpawnPickup(Position, ItemDefinition, NumberToSpawn, 0, SourceType, Source, OptionalOwnerPC ? OptionalOwnerPC->MyFortPawn : nullptr, bToss, bRandomRotation);
}


bool bHasItemVariantGuid2 = false;
bool bHasItemLevel = false;
bool bHasPickupInstigatorHandle = false;
bool bHasbUseItemPickupAnalyticEvent = false;
void UFortKismetLibrary::GiveItemToInventoryOwner(UObject* Object, FFrame& Stack)
{
	TScriptInterface<class IFortInventoryOwnerInterface> InventoryOwner;
	UFortItemDefinition* ItemDefinition;
	FGuid ItemVariantGuid;
	int32 NumberToGive;
	bool bNotifyPlayer;
	int32 ItemLevel = -1;
	int32 PickupInstigatorHandle = 0;
	bool bUseItemPickupAnalyticEvent = false;
	Stack.StepCompiledIn(&InventoryOwner);
	Stack.StepCompiledIn(&ItemDefinition);
	if (bHasItemVariantGuid2)
		Stack.StepCompiledIn(&ItemVariantGuid);
	Stack.StepCompiledIn(&NumberToGive);
	Stack.StepCompiledIn(&bNotifyPlayer);
	if (bHasItemLevel)
		Stack.StepCompiledIn(&ItemLevel);
	if (bHasPickupInstigatorHandle)
		Stack.StepCompiledIn(&PickupInstigatorHandle);
	if (bHasbUseItemPickupAnalyticEvent)
		Stack.StepCompiledIn(&bUseItemPickupAnalyticEvent);
	Stack.IncrementCode();

	auto PlayerController = (AFortPlayerControllerAthena*)InventoryOwner.ObjectPointer;
	auto ItemEntry = AFortInventory::MakeItemEntry(ItemDefinition, NumberToGive, ItemLevel);
	PlayerController->InternalPickup(ItemEntry);
	free(ItemEntry);
}


bool bHasItemVariantGuid = false;
bool bHasbForceRemoval = false;
void UFortKismetLibrary::K2_RemoveItemFromPlayer(UObject* Context, FFrame& Stack, int32* Ret)
{
	AFortPlayerControllerAthena* PlayerController;
	UFortItemDefinition* ItemDefinition;
	FGuid ItemVariantGuid{};
	int32 AmountToRemove;
	bool bForceRemoval = false;
	Stack.StepCompiledIn(&PlayerController);
	Stack.StepCompiledIn(&ItemDefinition);
	if (bHasItemVariantGuid)
		Stack.StepCompiledIn(&ItemVariantGuid);
	Stack.StepCompiledIn(&AmountToRemove);
	if (bHasbForceRemoval)
		Stack.StepCompiledIn(&bForceRemoval);
	Stack.IncrementCode();

	if (!PlayerController)
	{
		*Ret = 0;
		return;
	}

	auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemDefinition == ItemDefinition; }, FFortItemEntry::Size());
	if (!itemEntry)
	{
		*Ret = 0;
		return;
	}

	auto RemoveCount = max(AmountToRemove, 0);
	itemEntry->Count -= RemoveCount;

	if (AmountToRemove < 0 || itemEntry->Count <= 0 || ItemDefinition->IsA(UFortGadgetItemDefinition::StaticClass()))
	{
		RemoveCount += itemEntry->Count;
		PlayerController->WorldInventory->Remove(itemEntry->ItemGuid);
	}
	else
		PlayerController->WorldInventory->UpdateEntry(*itemEntry);

	*Ret = RemoveCount;
}

void UFortKismetLibrary::K2_RemoveItemFromPlayerByGuid(UObject* Context, FFrame& Stack, int32* Ret)
{
	class AFortPlayerControllerAthena* PlayerController;
	struct FGuid ItemGuid;
	int32 AmountToRemove;
	bool bForceRemoval;
	Stack.StepCompiledIn(&PlayerController);
	Stack.StepCompiledIn(&ItemGuid);
	Stack.StepCompiledIn(&AmountToRemove);
	Stack.StepCompiledIn(&bForceRemoval);
	Stack.IncrementCode();

	auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemGuid == ItemGuid; }, FFortItemEntry::Size());
	if (!ItemEntry)
	{
		*Ret = 0;
		return;
	}


	auto RemoveCount = max(AmountToRemove, 0);
	ItemEntry->Count -= RemoveCount;

	if (AmountToRemove < 0 || ItemEntry->Count <= 0 || ItemEntry->ItemDefinition->IsA(UFortGadgetItemDefinition::StaticClass())) {
		RemoveCount += ItemEntry->Count;
		PlayerController->WorldInventory->Remove(ItemEntry->ItemGuid);
	}
	else
		PlayerController->WorldInventory->UpdateEntry(*ItemEntry);

	*Ret = RemoveCount;
	return;
}

void UFortKismetLibrary::SpawnItemVariantPickupInWorld(UObject* Object, FFrame& Stack, AFortPickupAthena** Ret)
{
	UObject* WorldContextObject;
	Stack.StepCompiledIn(&WorldContextObject);
	auto& Params = Stack.StepCompiledInRef<FSpawnItemVariantParams>();
	Stack.IncrementCode();

	*Ret = AFortInventory::SpawnPickup(FSpawnItemVariantParams::HasPosition() ? Params.Position : Params.position, Params.WorldItemDefinition, Params.NumberToSpawn, 0, Params.SourceType, Params.Source, nullptr, Params.bToss, Params.bRandomRotation);
}

bool bHasOptionalLootTags = false;
bool bHasWorldContextObject2 = false;
void UFortKismetLibrary::PickLootDrops(UObject* Object, FFrame& Stack, bool* Ret)
{
	UObject* WorldContextObject;
	FName TierGroupName;
	int32 WorldLevel;
	int32 ForcedLootTier;
	FGameplayTagContainer OptionalLootTags{};

	if (bHasWorldContextObject2)
		Stack.StepCompiledIn(&WorldContextObject);
	auto& OutLootToDrop = Stack.StepCompiledInRef<TArray<FFortItemEntry>>();
	Stack.StepCompiledIn(&TierGroupName);
	Stack.StepCompiledIn(&WorldLevel);
	Stack.StepCompiledIn(&ForcedLootTier);
	if (bHasOptionalLootTags)
		Stack.StepCompiledIn(&OptionalLootTags);
	Stack.IncrementCode();

	auto LootDrops = UFortLootPackage::ChooseLootForContainer(TierGroupName, ForcedLootTier, WorldLevel);

	for (auto& LootDrop : LootDrops)
	{
		OutLootToDrop.Add(*LootDrop, FFortItemEntry::Size());
		free(LootDrop);
	}

	*Ret = LootDrops.Num() > 0;
}

void UFortKismetLibrary::Hook()
{
	auto GiveItemToInventoryOwnerFn = GetDefaultObj()->GetFunction("GiveItemToInventoryOwner");
	if (GiveItemToInventoryOwnerFn)
		for (auto& Param : GiveItemToInventoryOwnerFn->GetParamsNamed().NameOffsetMap)
		{
			if (Param.Name == "ItemVariantGuid")
				bHasItemVariantGuid2 = true;
			else if (Param.Name == "ItemLevel")
				bHasItemLevel = true;
			else if (Param.Name == "PickupInstigatorHandle")
				bHasPickupInstigatorHandle = true;
			else if (Param.Name == "bUseItemPickupAnalyticEvent")
				bHasbUseItemPickupAnalyticEvent = true;
		}
	Utils::ExecHook(GiveItemToInventoryOwnerFn, GiveItemToInventoryOwner);

	auto K2_RemoveItemFromPlayerFn = GetDefaultObj()->GetFunction("K2_RemoveItemFromPlayer");
	if (K2_RemoveItemFromPlayerFn)
		for (auto& Param : K2_RemoveItemFromPlayerFn->GetParamsNamed().NameOffsetMap)
		{
			if (Param.Name == "ItemVariantGuid")
				bHasItemVariantGuid = true;
			else if (Param.Name == "bForceRemoval")
				bHasbForceRemoval = true;
		}
	Utils::ExecHook(K2_RemoveItemFromPlayerFn, K2_RemoveItemFromPlayer);

	Utils::ExecHook(GetDefaultObj()->GetFunction("K2_RemoveItemFromPlayerByGuid"), K2_RemoveItemFromPlayerByGuid);

	auto K2_SpawnPickupInWorldFn = GetDefaultObj()->GetFunction("K2_SpawnPickupInWorld");
	if (K2_SpawnPickupInWorldFn)
		for (auto& Param : K2_SpawnPickupInWorldFn->GetParamsNamed().NameOffsetMap)
		{
			if (Param.Name == "bPickupOnlyRelevantToOwner")
				bHasbPickupOnlyRelevantToOwner = true;
			else if (Param.Name == "bToss")
				bHasbToss = true;
			else if (Param.Name == "bRandomRotation")
				bHasbRandomRotation = true;
			else if (Param.Name == "bBlockedFromAutoPickup")
				bHasbBlockedFromAutoPickup = true;
			else if (Param.Name == "PickupInstigatorHandle")
				bHasPickupInstigatorHandle2 = true;
			else if (Param.Name == "SourceType")
				bHasSourceType = true;
			else if (Param.Name == "Source")
				bHasSource = true;
			else if (Param.Name == "OptionalOwnerPC")
				bHasOptionalOwnerPC = true;
		}
	Utils::ExecHook(K2_SpawnPickupInWorldFn, K2_SpawnPickupInWorld);

	Utils::ExecHook(GetDefaultObj()->GetFunction("SpawnItemVariantPickupInWorld"), SpawnItemVariantPickupInWorld);

	auto PickLootDropsFn = GetDefaultObj()->GetFunction("PickLootDrops");

	if (PickLootDropsFn)
		for (auto& Param : PickLootDropsFn->GetParamsNamed().NameOffsetMap)
		{
			if (Param.Name == "OptionalLootTags")
			{
				bHasOptionalLootTags = true;
				break;
			} 
			else if (Param.Name == "WorldContextObject")
			{
				bHasWorldContextObject2 = true;
				break;
			}
		}
	Utils::ExecHook(PickLootDropsFn, PickLootDrops);
}