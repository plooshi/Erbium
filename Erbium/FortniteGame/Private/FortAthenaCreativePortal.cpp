#include "pch.h"
#include "../Public/FortAthenaCreativePortal.h"
#include "../Public/FortGameStateAthena.h"

void AFortMinigameSettingsBuilding::BeginPlay(AFortMinigameSettingsBuilding* Settings)
{
	if (Settings->HasSettingsVolume())
	{
	}

	return BeginPlayOG(Settings);
}

AFortAthenaCreativePortal* AFortAthenaCreativePortal::Create(AFortPlayerControllerAthena* PlayerController)
{
	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	auto PlayerState = PlayerController->PlayerState;

	AFortAthenaCreativePortal* Portal = nullptr;

	if (!GameState->CreativePortalManager->HasAvailablePortals())
	{
		static TArray<AFortAthenaCreativePortal*> AvailablePortals, UsedPortals;
		if (AvailablePortals.Num() == 0 && UsedPortals.Num() == 0)
			for (auto& Portal__Uncasted : GameState->CreativePortalManager->AllPortals)
			{
				auto Portal = (AFortAthenaCreativePortal*)Portal__Uncasted;

				if (Portal->OwningPlayer.ReplicationBytes.Num() > 0) UsedPortals.Add(Portal);
				else AvailablePortals.Add(Portal);
			}

		if (AvailablePortals.Num() > 0)
		{
			Portal = (AFortAthenaCreativePortal*)AvailablePortals[0];
			AvailablePortals.Remove(0);
			UsedPortals.Add(Portal);
		}
	}
	else
	{
		if (GameState->CreativePortalManager->AvailablePortals.Num() > 0)
		{
			Portal = (AFortAthenaCreativePortal*)GameState->CreativePortalManager->AvailablePortals[0];
			GameState->CreativePortalManager->AvailablePortals.Remove(0);
			GameState->CreativePortalManager->UsedPortals.Add(Portal);
		}
	}

	if (!Portal)
		return nullptr;

	printf("[Creative] Assigned portal %s to %s\n", Portal->Name.ToString().c_str(), PlayerController->Name.ToString().c_str());

	Portal->bIsPublishedPortal = false;
	Portal->OnRep_PublishedPortal();

	Portal->OwningPlayer = PlayerState->UniqueId;
	Portal->OnRep_OwningPlayer();
	Portal->bPortalOpen = true;
	Portal->OnRep_PortalOpen();

	auto RestrictedPlotDefinition = FindObject<UFortCreativeRealEstatePlotItemDefinition>(L"/Game/Playgrounds/Items/Plots/Temperate_Medium.Temperate_Medium");

	if (!RestrictedPlotDefinition)
		RestrictedPlotDefinition = FindObject<UFortCreativeRealEstatePlotItemDefinition>(L"/CR_Legacy/Playgrounds/Items/Plots/Temperate_Medium.Temperate_Medium");

	auto IslandPlayset = RestrictedPlotDefinition->BasePlayset.Get();

	auto LevelSaveComponent = (UFortLevelSaveComponent*)Portal->LinkedVolume->GetComponentByClass(UFortLevelSaveComponent::StaticClass());

	if (!LevelSaveComponent->RestrictedPlotDefinition)
	{
		LevelSaveComponent->AccountIdOfOwner = PlayerState->UniqueId;
		LevelSaveComponent->bIsLoaded = true;
		LevelSaveComponent->bLoadPlaysetFromPlot = true;
		LevelSaveComponent->bAutoLoadFromRestrictedPlotDefinition = true;
		LevelSaveComponent->RestrictedPlotDefinition = RestrictedPlotDefinition;
		if (LevelSaveComponent->HasPlotPermissions())
			LevelSaveComponent->PlotPermissions.Permission = 1;
		else
		{
			auto PermissionComponent = (UPlayspaceComponent_CreativeToolsPermission*)Portal->LinkedVolume->Playspace->GetComponentByClass(UPlayspaceComponent_CreativeToolsPermission::StaticClass());

			PermissionComponent->AccountIdOfOwner = PlayerState->UniqueId;
			PermissionComponent->PlotPermissions.Permission = 1;
		}
		LevelSaveComponent->OnRep_AccountIdOfOwner();
	}

	auto LevelStreamComponent = (UPlaysetLevelStreamComponent*)Portal->LinkedVolume->GetComponentByClass(UPlaysetLevelStreamComponent::StaticClass());
	LevelStreamComponent->SetPlayset(IslandPlayset);
	
	static auto SettingsMachineClass = FindObject<UClass>("/Game/Athena/Items/Gameplay/MinigameSettingsControl/MinigameSettingsMachine.MinigameSettingsMachine_C");
	auto SettingsMachine = UWorld::SpawnActor<AMinigameSettingsMachine_C>(SettingsMachineClass, FVector{}, {}, Portal->LinkedVolume);

	if (SettingsMachine)
	{
		if (SettingsMachine->HasSettingsVolume())
		{
			SettingsMachine->SettingsVolume = Portal->LinkedVolume;
			SettingsMachine->OnRep_SettingsVolume();
		}

		auto MinigameVolumeComponent = (UFortMinigameVolumeComponent*)Portal->LinkedVolume->GetComponentByClass(UFortMinigameVolumeComponent::StaticClass());
		if (MinigameVolumeComponent)
			MinigameVolumeComponent->CurrentMinigameSettingsMachine = SettingsMachine;
	}
	
	auto LoadPlayset = (void (*)(UPlaysetLevelStreamComponent*)) FindLoadPlayset();
	if (LoadPlayset)
		LoadPlayset(LevelStreamComponent);

	//PlayerController->CreativePlotLinkedVolume = Portal->LinkedVolume;
	//PlayerController->OnRep_CreativePlotLinkedVolume();

	Portal->LinkedVolume->VolumeState = 3;
	Portal->LinkedVolume->OnRep_VolumeState();

	PlayerController->OwnedPortal = Portal;

	printf("[Creative] Setup portal!\n");

	PlayerController->CreativePlotLinkedVolume = PlayerController->GetCurrentVolume();
	PlayerController->OnRep_CreativePlotLinkedVolume();

	return nullptr;
}

void AFortAthenaCreativePortal::TeleportPlayerToLinkedVolume(UObject* Context, FFrame& Stack)
{
	AFortPlayerPawnAthena* PlayerPawn = nullptr;
	bool bUseSpawnTags;

	Stack.StepCompiledIn(&PlayerPawn);
	Stack.StepCompiledIn(&bUseSpawnTags);
	Stack.IncrementCode();
	auto Portal = (AFortAthenaCreativePortal*)Context;

	if (!PlayerPawn)
		return;

	auto PlayerController = (AFortPlayerControllerAthena*)PlayerPawn->Controller;

	// credit to andrew

	static auto CreativePhone = FindObject<UFortWeaponItemDefinition>(L"/Game/Athena/Items/Weapons/Prototype/WID_CreativeTool.WID_CreativeTool");

	auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{
			return entry.ItemDefinition == CreativePhone;
		}, FFortItemEntry::Size());

	if (!ItemEntry)
	{
		PlayerController->WorldInventory->GiveItem(CreativePhone, 1, AFortInventory::GetStats(CreativePhone)->ClipSize);
		PlayerController->ClientCreativePhoneCreated();
	}

	if (PlayerController->HasbIsCreativeQuickbarEnabled())
		PlayerController->bIsCreativeQuickbarEnabled = true;
	if (PlayerController->HasbIsCreativeQuickmenuEnabled())
		PlayerController->bIsCreativeQuickmenuEnabled = true;
	if (PlayerController->HasbIsCreativeModeEnabled())
	{
		PlayerController->bIsCreativeModeEnabled = true;
		PlayerController->OnRep_IsCreativeModeEnabled();
	}

	auto Location = Portal->LinkedVolume->K2_GetActorLocation();
	Location.Z = 10000;

	PlayerController->CreativePlotLinkedVolume = Portal->LinkedVolume;
	PlayerController->OnRep_CreativePlotLinkedVolume();

	PlayerPawn->K2_TeleportTo(Location, FRotator());
	PlayerPawn->BeginSkydiving(false);
}

void AFortAthenaCreativePortal::Hook()
{
	if (!GetDefaultObj())
		return;
	
	Utils::ExecHook(GetDefaultObj()->GetFunction("TeleportPlayerToLinkedVolume"), TeleportPlayerToLinkedVolume);
}

void AFortMinigameSettingsBuilding::Hook()
{
	if (!GetDefaultObj())
		return;
	
	//Utils::Hook(FindMinigameSettingsBuilding__BeginPlay(), BeginPlay, BeginPlayOG);
}
