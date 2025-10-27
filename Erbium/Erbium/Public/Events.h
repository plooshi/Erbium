#pragma once
#include "../../pch.h"

struct FEventFunction
{
    bool bIsLoaderFunction;
    const wchar_t* FunctionPath;
};

struct FEvent
{
    const wchar_t* LoaderClass;
    const wchar_t* ScriptingClass;
    std::vector<FEventFunction> EventFunctions;
    double EventVersion;
    const wchar_t* LoaderFuncPath;
};

class Events
{
public:
#ifndef CLIENT
	static inline std::vector<FEvent> EventsArray =
	{
		FEvent
		{
			nullptr,
			L"/Game/Athena/Maps/Test/Events/BP_GeodeScripting.BP_GeodeScripting_C",
			{
				FEventFunction{ false, L"/Game/Athena/Maps/Test/Events/BP_GeodeScripting.BP_GeodeScripting_C.LaunchSequence" }
			},
			4.5,
			nullptr
		},

		FEvent
		{
			L"/Game/Athena/Prototype/Blueprints/Cube/CUBE.CUBE_C",
			L"/Game/Athena/Events/BP_Athena_Event_Components.BP_Athena_Event_Components_C",
			{
				FEventFunction{ false, L"/Game/Athena/Events/BP_Athena_Event_Components.BP_Athena_Event_Components_C.Final" },
				FEventFunction{ true, L"/Game/Athena/Prototype/Blueprints/Cube/CUBE.CUBE_C.Final" }
			},
			5.30,
			nullptr
		},

		FEvent
		{
			L"/Game/Athena/Prototype/Blueprints/Island/BP_Butterfly.BP_Butterfly_C",
			nullptr,
			{
				FEventFunction{ true, L"/Game/Athena/Prototype/Blueprints/Island/BP_Butterfly.BP_Butterfly_C.ButterflySequence" }
			},
			6.21,
			L"/Game/Athena/Prototype/Blueprints/Island/BP_Butterfly.BP_Butterfly_C.LoadButterflySublevel"
		},

		FEvent
		{
			nullptr,
			L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C",
			{
				FEventFunction{ false, L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C.FinalSequence" }
			},
			8.51,
			L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C.LoadSnowLevel"
		},

		FEvent
		{
			L"/Game/Athena/Prototype/Blueprints/Cattus/BP_CattusDoggus_Scripting.BP_CattusDoggus_Scripting_C",
			nullptr,
			{
				FEventFunction{ true, L"/Game/Athena/Prototype/Blueprints/Cattus/BP_CattusDoggus_Scripting.BP_CattusDoggus_Scripting_C.OnReady_C11CA7624A74FBAEC54753A3C2BD4506" },
				FEventFunction{ true, L"/Game/Athena/Prototype/Blueprints/Cattus/BP_CattusDoggus_Scripting.BP_CattusDoggus_Scripting_C.startevent" }
			},
			9.41,
			L"/Game/Athena/Prototype/Blueprints/Cattus/BP_CattusDoggus_Scripting.BP_CattusDoggus_Scripting_C.LoadCattusLevel"
		},

		FEvent
		{
			nullptr,
			L"/Game/Athena/Prototype/Blueprints/NightNight/BP_NightNight_Scripting.BP_NightNight_Scripting_C",
			{
				FEventFunction{ false, L"/Game/Athena/Prototype/Blueprints/NightNight/BP_NightNight_Scripting.BP_NightNight_Scripting_C.OnReady_D0847F7B4E80F01E77156AA4E7131AF6" },
				FEventFunction{ false, L"/Game/Athena/Prototype/Blueprints/NightNight/BP_NightNight_Scripting.BP_NightNight_Scripting_C.startevent" }
			},
			10.40,
			L"/Game/Athena/Prototype/Blueprints/NightNight/BP_NightNight_Scripting.BP_NightNight_Scripting_C.LoadNightNightLevel"
		},

		FEvent
		{
			L"/Junior/Blueprints/BP_Junior_Loader.BP_Junior_Loader_C",
			L"/Junior/Blueprints/BP_Junior_Scripting.BP_Junior_Scripting_C",
			{
				FEventFunction{ false, L"/Junior/Blueprints/BP_Event_Master_Scripting.BP_Event_Master_Scripting_C.OnReady_872E6C4042121944B78EC9AC2797B053" },
				FEventFunction{ false, L"/Junior/Blueprints/BP_Junior_Scripting.BP_Junior_Scripting_C.startevent" }
			},
			14.60,
			L"/Junior/Blueprints/BP_Junior_Loader.BP_Junior_Loader_C.LoadJuniorLevel"
		},

		FEvent
		{
			L"/CycloneJerky/Gameplay/BP_Jerky_Loader.BP_Jerky_Loader_C",
			L"/CycloneJerky/Gameplay/BP_Jerky_Scripting.BP_Jerky_Scripting_C",
			{
				//FEventFunction{ false, L"/CycloneJerky/Gameplay/BP_Jerky_Scripting.BP_Jerky_Scripting_C.OnReady_093B6E664C060611B28F79B5E7052A39" },
				//FEventFunction{ true, L"/CycloneJerky/Gameplay/BP_Jerky_Loader.BP_Jerky_Loader_C.OnReady_7FE9744D479411040654F5886C078D08" },
				FEventFunction{ true, L"/CycloneJerky/Gameplay/BP_Jerky_Loader.BP_Jerky_Loader_C.startevent" }
			},
			12.41,
			nullptr
		},
		FEvent
		{
			nullptr,
			L"/Buffet/Gameplay/Blueprints/Buffet_SpecialEventScript.Buffet_SpecialEventScript_C",
			{
				FEventFunction{ false, L"/Script/SpecialEventGameplayRuntime.SpecialEventScript.StartEventAtIndex" }
			},
			17.30,
			nullptr
		},
		FEvent
		{
			nullptr,
			L"/Guava/Gameplay/BP_Guava_SpecialEventScript.BP_Guava_SpecialEventScript_C",
			{
				FEventFunction{ false, L"/Script/SpecialEventGameplayRuntime.SpecialEventScript.StartEventAtIndex" }
			},
			18.40,
			nullptr
		},
		FEvent
		{
			nullptr,
			L"/Radish/Gameplay/BP_Radish_Special_EventScript.BP_Radish_Special_EventScript_C",
			{
				FEventFunction{ false, L"/Script/SpecialEventGameplayRuntime.SpecialEventScript.StartEventAtIndex" }
			},
			22.40,
			nullptr
		}
	};

#else

	static inline std::vector<FEvent> EventsArray =
	{};
#endif
    static void StartEvent();
};