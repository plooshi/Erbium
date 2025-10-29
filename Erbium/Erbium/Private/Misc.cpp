#include "pch.h"
#include "../Public/Misc.h"
#include "../Public/Finders.h"
#include <algorithm>
#include "../Public/Configuration.h"
#include "../../FortniteGame/Public/FortPlayerControllerAthena.h"
#include "../../Engine/Public/NetDriver.h"

int Misc::GetNetMode() 
{
	return 1;
}


void* Misc::SendRequestNow(void* Arg1, void* MCPData, int)
{
	if (VersionInfo.EngineVersion < 4.23)
		*(int*)(__int64(MCPData) + (VersionInfo.FortniteVersion >= 4.2 ? 0x28 : 0x60)) = 3; // CXC_Public

	return SendRequestNowOG(Arg1, MCPData, 3); // CXC_Public
}


float Misc::GetMaxTickRate(UEngine* Engine, float DeltaTime, bool bAllowFrameRateSmoothing)
{
	// improper, DS is supposed to do hitching differently
	return std::clamp(1.f / DeltaTime, 1.f, FConfiguration::MaxTickRate);
}

uint32 Misc::CheckCheckpointHeartBeat()
{
	return -1;
}

void Misc::ApplyHomebaseEffectsOnPlayerSetup(
	__int64* a1,
	__int64 a2,
	__int64 a3,
	__int64 a4,
	UObject* a5,
	char a6,
	unsigned __int8 a7)
{
	static auto ItemDefOffset = a5->GetOffset("ItemDefinition");
	static auto Commando = FindObject<UObject>(L"/Game/Athena/Heroes/HID_001_Athena_Commando_F.HID_001_Athena_Commando_F");
	static auto Commando2 = FindObject<UObject>(L"/Game/Athena/Heroes/HID_Commando_Athena_01.HID_Commando_Athena_01");
	GetFromOffset<const UObject*>(a5, ItemDefOffset) = Commando ? Commando : Commando2;

	return ApplyHomebaseEffectsOnPlayerSetupOG(a1, a2, a3, a4, a5, a6, a7);
}

bool bEOREnabled = false;
inline void* (*SelectResetOG)(void*) = nullptr;
inline void* (*SelectEditOG)(void*) = nullptr;
inline char (*CompleteBuildingEditInteraction)(void*) = nullptr;

void* SelectEdit(void* a1)
{
	void* result = SelectEditOG(a1);

	if (bEOREnabled)
		CompleteBuildingEditInteraction(a1);

	return result;
}

void* SelectReset(void* a1)
{
	void* result = SelectResetOG(a1);

	if (bEOREnabled)
		CompleteBuildingEditInteraction(a1);

	return result;
}

void ClientThread()
{
	bool bPressed = false;
	while (true)
	{
		if (!bPressed && GetAsyncKeyState(VK_F3))
		{
			bPressed = true;

			bEOREnabled ^= 1;
		}
		else if (!bPressed && GetAsyncKeyState(VK_F2))
		{
			bPressed = true;
			//bEnableResetOnRelease ^= 1;
			auto& LocalPlayers = UWorld::GetWorld()->OwningGameInstance->LocalPlayers;

			if (LocalPlayers.Num() > 0)
			{
				auto PlayerController = (AFortPlayerControllerAthena*) LocalPlayers[0]->PlayerController;

				if (!PlayerController->CheatManager)
				{
					PlayerController->CheatManager = UGameplayStatics::SpawnObject(PlayerController->CheatClass, PlayerController);
					PlayerController->CheatManager->ObjectFlags &= ~0x1000000;
				}
			}
		}
		else if (!GetAsyncKeyState(VK_F3) && !GetAsyncKeyState(VK_F2))
			bPressed = false;

		Sleep(33); // thread runs at 30tps	
	}
}

void Misc::InitClient()
{
	UEngine::GetEngine()->GameViewport->ViewportConsole = UGameplayStatics::SpawnObject(UEngine::GetEngine()->ConsoleClass, UEngine::GetEngine()->GameViewport);

	auto ArenaUI = UKismetStringLibrary::Conv_StringToName(FString(L"/Game/UI/Competitive/Arena/ArenaScoringHUD.ArenaScoringHUD_C"));
	FUIExtension ArenaUIExtension{};
	ArenaUIExtension.Slot = 0;
	ArenaUIExtension.WidgetClass.ObjectID.AssetPathName = ArenaUI;

	auto ShowdownUI = UKismetStringLibrary::Conv_StringToName(FString(L"/Game/UI/Frontend/Showdown/ShowdownScoringHUD.ShowdownScoringHUD_C"));
	FUIExtension ShowdownUIExtension{};
	ShowdownUIExtension.Slot = 0;
	ShowdownUIExtension.WidgetClass.ObjectID.AssetPathName = ShowdownUI;

	auto AIKillsUI = UKismetStringLibrary::Conv_StringToName(FString(L"/Game/Athena/HUD/AthenaAIKillsWidget.AthenaAIKillsWidget_C"));
	FUIExtension AIKillsUIExtension{};
	AIKillsUIExtension.Slot = 2;
	AIKillsUIExtension.WidgetClass.ObjectID.AssetPathName = AIKillsUI;

	TArray<FUIExtension> ArenaExtensions, ShowdownExtensions;
	ArenaExtensions.Add(ArenaUIExtension);
	ShowdownExtensions.Add(ShowdownUIExtension);
	ShowdownExtensions.Add(AIKillsUIExtension);
		
	/*auto PlaylistClass = FindClass("FortPlaylistAthena");

	for (int i = 0; i < TUObjectArray::Num(); i++)
	{
		auto Object = TUObjectArray::GetObjectByIndex(i);
		if (Object && Object->IsA((UClass*)PlaylistClass))
		{
			auto Playlist = (UFortPlaylistAthena*)Object;

			auto Name = Object->Name.ToString();
			if (Name.contains("Showdown"))
				Playlist->UIExtensions = Name.contains("ShowdownAlt") ? ArenaExtensions : ShowdownExtensions;
		}
	}*/

	if (VersionInfo.FortniteVersion < 20)
	{
		auto SelectEditAddr = Memcury::Scanner::FindStringRef(L"EditModeInputComponent0").ScanFor({ 0x48, 0x8D, 0x05 }, true, 1).RelativeOffset(3).GetAs<void*>();
		auto SelectResetAddr = Memcury::Scanner::FindStringRef(L"EditModeInputComponent0").ScanFor({ 0x48, 0x8D, 0x05 }, true, 2).RelativeOffset(3).GetAs<void*>();

		auto sRef = Memcury::Scanner::FindStringRef("CompleteBuildingEditInteraction", true, VersionInfo.EngineVersion >= 4.27).Get();
		uintptr_t CompleteBuildingEditInteractionLea = 0;

		for (int i = 1; i < 2000; i++)
		{
			if (*(uint8_t*)(sRef - i) == 0x4C && *(uint8_t*)(sRef - i + 1) == 0x8D)
			{
				CompleteBuildingEditInteractionLea = sRef - i;
				break;
			}
			else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8D)
			{
				CompleteBuildingEditInteractionLea = sRef - i;
				break;
			}
		}

		CompleteBuildingEditInteraction = (char (*)(void*)) Memcury::Scanner(CompleteBuildingEditInteractionLea).RelativeOffset(3).Get();

		MH_Initialize();

		if (VersionInfo.FortniteVersion < 11)
			MH_CreateHook(SelectEditAddr, SelectEdit, (LPVOID*)&SelectEditOG);
		if (VersionInfo.FortniteVersion < 24.40)
			MH_CreateHook(SelectResetAddr, SelectReset, (LPVOID*)&SelectResetOG);

		MH_EnableHook(MH_ALL_HOOKS);
	}

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ClientThread, 0, 0, 0);
}

class UIpNetDriver : public UNetDriver
{
public:
	UCLASS_COMMON_MEMBERS(UIpNetDriver);
};

void PatchAllNetModes()
{
	Memcury::PE::Address add{ nullptr };

	const auto sizeOfImage = Memcury::PE::GetNTHeaders()->OptionalHeader.SizeOfImage;
	auto patternBytes = Memcury::ASM::pattern2bytes("48 8B 01 FF 90 ? ? ? ? 84 C0 0F 85 ? ? ? ? B8 03 00 00 00");
	const auto scanBytes = reinterpret_cast<std::uint8_t*>(Memcury::PE::GetModuleBase());

	const auto s = patternBytes.size();
	const auto d = patternBytes.data();

	for (auto i = 0ul; i < sizeOfImage - s; ++i)
	{
		bool found = true;
		for (auto j = 0ul; j < s; ++j)
		{
			if (scanBytes[i + j] != d[j] && d[j] != -1)
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			Utils::Patch<uint32>(__int64(scanBytes + i) + 0x12, 1);
		}
	}
}

bool RetFalse()
{
	return false;
}

void Misc::Hook()
{
	Utils::Hook(FindGetNetMode(), GetNetMode);
	if (VersionInfo.FortniteVersion >= 25 && VersionInfo.FortniteVersion < 28)
	{
		Utils::Hook(__int64(UKismetSystemLibrary::GetDefaultObj()->GetFunction("IsDedicatedServer")->GetImpl()), GetNetMode);
		Utils::Hook(__int64(UKismetSystemLibrary::GetDefaultObj()->GetFunction("IsServer")->GetImpl()), GetNetMode);
		PatchAllNetModes();
	}
	Utils::Hook(FindSendRequestNow(), SendRequestNow, SendRequestNowOG);
	Utils::Hook(FindGetMaxTickRate(), GetMaxTickRate);
	if (VersionInfo.FortniteVersion >= 17)
		Utils::Hook(Memcury::Scanner::FindPattern("48 89 5C 24 10 48 89 6C 24 20 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B F9").Get(), CheckCheckpointHeartBeat);

	if (VersionInfo.EngineVersion < 4.20)
	{
		auto ApplyHomebaseEffectsOnPlayerSetupAddr = Memcury::Scanner::FindPattern("40 55 53 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 00 4C 8B").Get();

		Utils::Hook(ApplyHomebaseEffectsOnPlayerSetupAddr, ApplyHomebaseEffectsOnPlayerSetup, ApplyHomebaseEffectsOnPlayerSetupOG);
	}
	if (VersionInfo.FortniteVersion >= 26 && VersionInfo.FortniteVersion < 28)
	{
		Utils::Hook(Memcury::Scanner::FindPattern("48 89 5C ? ? 57 48 83 EC ? 48 8B D1 48 85 C9 74 ?").Get(), RetFalse);
	}
}
