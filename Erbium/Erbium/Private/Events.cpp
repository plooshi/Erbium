#include "pch.h"
#include "../Public/Events.h"
#include <thread>
#include "FortniteGame/Public/FortGameModeAthena.h"

void Events::StartEvent()
{
	if (VersionInfo.FortniteVersion < 4.4) return; // no other events from what i know of?

	static std::vector<FEvent> EventsArray =
	{
		FEvent
		{
			.LoaderClass = L"",
			.ScriptingClass = L"/Game/Athena/Maps/Test/Events/BP_GeodeScripting.BP_GeodeScripting_C",
			.EventFunctions = 
			{
				FEventFunction{ .bIsLoaderFunction = false, .FunctionPath = L"/Game/Athena/Maps/Test/Events/BP_GeodeScripting.BP_GeodeScripting_C.LaunchSequence" }
			},
			.EventVersion = 4.5,
			.LoaderFuncPath = L""
		},

		FEvent
		{
			.LoaderClass = L"/Game/Athena/Prototype/Blueprints/Cube/CUBE.CUBE_C",
			.ScriptingClass = L"/Game/Athena/Events/BP_Athena_Event_Components.BP_Athena_Event_Components_C",
			.EventFunctions = 
			{
				FEventFunction{ .bIsLoaderFunction = false, .FunctionPath = L"/Game/Athena/Events/BP_Athena_Event_Components.BP_Athena_Event_Components_C.Final" },
				FEventFunction{ .bIsLoaderFunction = true, .FunctionPath = L"/Game/Athena/Prototype/Blueprints/Cube/CUBE.CUBE_C.Final" }
			},
			.EventVersion = 5.30,
			.LoaderFuncPath = L""
		},

		FEvent
		{
			.LoaderClass = L"/Game/Athena/Prototype/Blueprints/Island/BP_Butterfly.BP_Butterfly_C",
			.ScriptingClass = L"",
			.EventFunctions = 
			{
				FEventFunction{ .bIsLoaderFunction = true, .FunctionPath = L"/Game/Athena/Prototype/Blueprints/Island/BP_Butterfly.BP_Butterfly_C.ButterflySequence" }
			},
			.EventVersion = 6.21,
			.LoaderFuncPath = L"/Game/Athena/Prototype/Blueprints/Island/BP_Butterfly.BP_Butterfly_C.LoadButterflySublevel"
		},

		FEvent
		{
			.LoaderClass = L"",
			.ScriptingClass = L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C",
			.EventFunctions = 
			{
				FEventFunction{ .bIsLoaderFunction = false, .FunctionPath = L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C.FinalSequence" }
			},
			.EventVersion = 8.51,
			.LoaderFuncPath = L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C.LoadSnowLevel"
		},

		FEvent
		{
			.LoaderClass = L"/Game/Athena/Prototype/Blueprints/Cattus/BP_CattusDoggus_Scripting.BP_CattusDoggus_Scripting_C",
			.ScriptingClass = L"",
			.EventFunctions = 
			{
				FEventFunction{ .bIsLoaderFunction = true, .FunctionPath = L"/Game/Athena/Prototype/Blueprints/Cattus/BP_CattusDoggus_Scripting.BP_CattusDoggus_Scripting_C.OnReady_C11CA7624A74FBAEC54753A3C2BD4506" },
				FEventFunction{ .bIsLoaderFunction = true, .FunctionPath = L"/Game/Athena/Prototype/Blueprints/Cattus/BP_CattusDoggus_Scripting.BP_CattusDoggus_Scripting_C.startevent" }
			},
			.EventVersion = 9.41,
			.LoaderFuncPath = L"/Game/Athena/Prototype/Blueprints/Cattus/BP_CattusDoggus_Scripting.BP_CattusDoggus_Scripting_C.LoadCattusLevel"
		},

		FEvent
		{
			.LoaderClass = L"",
			.ScriptingClass = L"/Game/Athena/Prototype/Blueprints/NightNight/BP_NightNight_Scripting.BP_NightNight_Scripting_C",
			.EventFunctions = 
			{
				FEventFunction{ .bIsLoaderFunction = false, .FunctionPath = L"/Game/Athena/Prototype/Blueprints/NightNight/BP_NightNight_Scripting.BP_NightNight_Scripting_C.OnReady_D0847F7B4E80F01E77156AA4E7131AF6" },
				FEventFunction{ .bIsLoaderFunction = false, .FunctionPath = L"/Game/Athena/Prototype/Blueprints/NightNight/BP_NightNight_Scripting.BP_NightNight_Scripting_C.startevent" }
			},
			.EventVersion = 10.40,
			.LoaderFuncPath = L"/Game/Athena/Prototype/Blueprints/NightNight/BP_NightNight_Scripting.BP_NightNight_Scripting_C.LoadNightNightLevel"
		},

		FEvent
		{
			.LoaderClass = L"/Junior/Blueprints/BP_Junior_Loader.BP_Junior_Loader_C",
			.ScriptingClass = L"/Junior/Blueprints/BP_Junior_Scripting.BP_Junior_Scripting_C",
			.EventFunctions = 
			{
				FEventFunction{ .bIsLoaderFunction = false, .FunctionPath = L"/Junior/Blueprints/BP_Event_Master_Scripting.BP_Event_Master_Scripting_C.OnReady_872E6C4042121944B78EC9AC2797B053" },
				FEventFunction{ .bIsLoaderFunction = false, .FunctionPath = L"/Junior/Blueprints/BP_Junior_Scripting.BP_Junior_Scripting_C.startevent" }
			},
			.EventVersion = 14.60,
			.LoaderFuncPath = L"/Junior/Blueprints/BP_Junior_Loader.BP_Junior_Loader_C.LoadJuniorLevel"
		},

		FEvent
		{
			.LoaderClass = L"/CycloneJerky/Gameplay/BP_Jerky_Loader.BP_Jerky_Loader_C",
			.ScriptingClass = L"",
			.EventFunctions = 
			{
				FEventFunction{ .bIsLoaderFunction = false, .FunctionPath = L"/CycloneJerky/Gameplay/BP_Jerky_Loader.BP_Jerky_Loader_C.OnReady_7FE9744D479411040654F5886C078D08" },
				FEventFunction{ .bIsLoaderFunction = true, .FunctionPath = L"/CycloneJerky/Gameplay/BP_Jerky_Loader.BP_Jerky_Loader_C.startevent" }
			},
			.EventVersion = 12.41,
			.LoaderFuncPath = L"/CycloneJerky/Gameplay/BP_Jerky_Loader.BP_Jerky_Loader_C.LoadJerkyLevel"
		}
	};


	auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	auto Playlist = VersionInfo.FortniteVersion >= 4.0 ? GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData : nullptr;

	for (auto& Event : EventsArray)
	{
		if (Event.EventVersion != VersionInfo.FortniteVersion) 
			continue;

		UObject* LoaderObject = nullptr;
		if (const UClass* LoaderClass = Utils::FindObject<UClass>(Event.LoaderClass))
		{
			auto AllLoaders = Utils::GetAll(LoaderClass);
			LoaderObject = AllLoaders.Num() > 0 ? AllLoaders[0] : nullptr;
		}

		if (Event.LoaderFuncPath != L"" && LoaderObject)
			if (const UFunction* LoaderFunction = Utils::FindObject<UFunction>(Event.LoaderFuncPath))
			{
				int Param = 1;
				LoaderObject->ProcessEvent(const_cast<UFunction*>(LoaderFunction), &Param);
				printf("StartEvent: Ready!\n");
			}
			else 
				printf("StartEvent: failed to prepare event!\n");

		UObject* ScriptingObject = nullptr;
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
				printf("StartEvent: failed to find func: %ls\n", EventFunction.FunctionPath.c_str()); 
				continue; 
			}

			UObject* Target = EventFunction.bIsLoaderFunction ? LoaderObject : ScriptingObject;
			if (EventFunction.FunctionPath.find(L"OnReady") != std::string::npos)
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