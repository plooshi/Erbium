#include "pch.h"
#include "../Public/Events.h"
#include <thread>
#include "../../FortniteGame/Public/FortGameModeAthena.h"

void Events::StartEvent()
{
	if (VersionInfo.FortniteVersion < 4.4) return; // no other events from what i know of?


	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto Playlist = VersionInfo.FortniteVersion >= 4.0 ? GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData : nullptr;

	for (auto& Event : EventsArray)
	{
		if (Event.EventVersion != VersionInfo.FortniteVersion) 
			continue;

		UObject* LoaderObject = nullptr;
		if (Event.LoaderClass)
			if (const UClass* LoaderClass = Utils::FindObject<UClass>(Event.LoaderClass))
			{
				auto AllLoaders = Utils::GetAll(LoaderClass);
				LoaderObject = AllLoaders.Num() > 0 ? AllLoaders[0] : nullptr;
			}

		if (Event.LoaderFuncPath != nullptr && LoaderObject)
			if (const UFunction* LoaderFunction = Utils::FindObject<UFunction>(Event.LoaderFuncPath))
			{
				int Param = 1;
				LoaderObject->ProcessEvent(const_cast<UFunction*>(LoaderFunction), &Param);
				printf("StartEvent: Ready!\n");
			}
			else 
				printf("StartEvent: failed to prepare event!\n");

		UObject* ScriptingObject = nullptr;
		if (Event.ScriptingClass)
			if (const UClass* ScriptingClass = Utils::FindObject<UClass>(Event.ScriptingClass))
			{
				auto AllLoaders = Utils::GetAll(ScriptingClass);
				ScriptingObject = AllLoaders.Num() > 0 ? AllLoaders[0] : nullptr;
			}

		for (auto& EventFunction : Event.EventFunctions)
		{
			const UFunction* Function = Utils::FindObject<UFunction>(EventFunction.FunctionPath);
			if (!Function) 
			{ 
				printf("StartEvent: failed to find func: %ls\n", EventFunction.FunctionPath); 
				continue; 
			}

			UObject* Target = EventFunction.bIsLoaderFunction ? LoaderObject : ScriptingObject;
			if (wcsstr(EventFunction.FunctionPath, L"OnReady"))
			{
				struct { UObject* GameState; const UFortPlaylistAthena* Playlist; FGameplayTagContainer PlaylistContextTags; }
				Params{ GameMode->GameState, Playlist, FGameplayTagContainer() };
				Target->ProcessEvent(const_cast<UFunction*>(Function), &Params);
			}
			else 
				Target->ProcessEvent(const_cast<UFunction*>(Function), nullptr);

			if (VersionInfo.FortniteVersion == 8.51 && ScriptingObject && !EventFunction.bIsLoaderFunction)
			{
				std::thread([ScriptingObject] {
					std::this_thread::sleep_for(std::chrono::minutes(3));

					auto SetUnvaultFn = Utils::FindObject<UFunction>(L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C.SetUnvaultItemName");
					auto PillarsFn = Utils::FindObject<UFunction>(L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C.PillarsConcluded");
					FName Name = UKismetStringLibrary::Conv_StringToName(FString(L"DrumGun"));

					ScriptingObject->ProcessEvent(const_cast<UFunction*>(SetUnvaultFn), &Name);
					ScriptingObject->ProcessEvent(const_cast<UFunction*>(PillarsFn), &Name);
				}).detach();
			}
		}

		printf("StartEvent: Started!\n");
		return;
	}

	printf("StartEvent: failed to start.\n");
}