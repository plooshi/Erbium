#include "pch.h"
#include "../Public/FortAthenaCreativePortal.h"
#include "../Public/FortGameStateAthena.h"

AFortAthenaCreativePortal* AFortAthenaCreativePortal::Create(AFortPlayerControllerAthena* PlayerController)
{
	auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
	auto PlayerState = PlayerController->PlayerState;

	AFortAthenaCreativePortal* Portal = nullptr;

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

	if (!Portal)
		return nullptr;

	printf("[Creative] Assigned portal %s to %s\n", Portal->Name.ToString().c_str(), PlayerController->Name.ToString().c_str());

	Portal->bIsPublishedPortal = false;
	Portal->OnRep_PublishedPortal();

	Portal->OwningPlayer = PlayerState->UniqueId;
	Portal->OnRep_OwningPlayer();
	Portal->bPortalOpen = true;
	Portal->OnRep_PortalOpen();
	Portal->PlayersReady.Add(PlayerState->UniqueId);
	Portal->OnRep_PlayersReady();

	Portal->bUserInitiatedLoad = true;
	Portal->bInErrorState = false;

	Portal->LinkedVolume->bNeverAllowSaving = false;
	Portal->LinkedVolume->VolumeState = 3;
	Portal->LinkedVolume->OnRep_VolumeState();

	auto IslandPlayset = FindObject<UFortPlaysetItemDefinition>(L"/Game/Playsets/PID_Playset_60x60_Composed.PID_Playset_60x60_Composed");
	Portal->LinkedVolume->CurrentPlayset = IslandPlayset;
	Portal->LinkedVolume->OnRep_CurrentPlayset();


	auto LevelSaveComponent = (UFortLevelSaveComponent*)Portal->LinkedVolume->GetComponentByClass(UFortLevelSaveComponent::StaticClass());

	if (!LevelSaveComponent->RestrictedPlotDefinition)
	{
		LevelSaveComponent->AccountIdOfOwner = PlayerState->UniqueId;
		LevelSaveComponent->bIsLoaded = true;
		LevelSaveComponent->bLoadPlaysetFromPlot = true;
		LevelSaveComponent->bAutoLoadFromRestrictedPlotDefinition = true;
		LevelSaveComponent->RestrictedPlotDefinition = FindObject<UObject>(L"/Game/Playgrounds/Items/Plots/Temperate_Medium.Temperate_Medium");
	}

	auto LevelStreamComponent = (UPlaysetLevelStreamComponent*)Portal->LinkedVolume->GetComponentByClass(UPlaysetLevelStreamComponent::StaticClass());

	LevelStreamComponent->CurrentPlayset = IslandPlayset;
	LevelStreamComponent->SetPlayset(IslandPlayset);
	//LevelStreamComponent->ClientPlaysetData.bValid = true;
	//LevelStreamComponent->OnRep_ClientPlaysetData();
	LevelStreamComponent->bAutoLoadLevel = true;
	LevelStreamComponent->bAutoActivate = true;

	auto LoadPlayset = (void (*)(UPlaysetLevelStreamComponent*)) FindLoadPlayset();
	LoadPlayset(LevelStreamComponent);

	PlayerController->CreativePlotLinkedVolume = Portal->LinkedVolume;
	PlayerController->OnRep_CreativePlotLinkedVolume();

	PlayerController->OwnedPortal = Portal;

	printf("[Creative] Setup portal!\n");

	return nullptr;
}