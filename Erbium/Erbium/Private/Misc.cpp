#include "pch.h"
#include "../Public/Misc.h"
#include "../Public/Finders.h"
#include <algorithm>
#include "../Public/Configuration.h"
#include "../../FortniteGame/Public/FortPlayerControllerAthena.h"

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


float Misc::GetMaxTickRate(UEngine* Engine, float DeltaTime, bool bAllowFrameRateSmoothing) {
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
	static auto Commando = Utils::FindObject<UObject>(L"/Game/Athena/Heroes/HID_001_Athena_Commando_F.HID_001_Athena_Commando_F");
	static auto Commando2 = Utils::FindObject<UObject>(L"/Game/Athena/Heroes/HID_Commando_Athena_01.HID_Commando_Athena_01");
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

				PlayerController->CheatManager = UGameplayStatics::SpawnObject(PlayerController->CheatClass, PlayerController);
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

	auto SelectEditAddr = Memcury::Scanner::FindStringRef(L"EditModeInputComponent0").ScanFor({ 0x48, 0x8D, 0x05 }, true, 1).RelativeOffset(3).GetAs<void*>();
	auto SelectResetAddr = Memcury::Scanner::FindStringRef(L"EditModeInputComponent0").ScanFor({ 0x48, 0x8D, 0x05 }, true, 2).RelativeOffset(3).GetAs<void*>();

	auto sRef = Memcury::Scanner::FindStringRef("CompleteBuildingEditInteraction", true).Get();
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

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ClientThread, 0, 0, 0);
}

void Misc::Hook()
{
	Utils::Hook(FindGetNetMode(), GetNetMode);
	Utils::Hook(FindSendRequestNow(), SendRequestNow, SendRequestNowOG);
	Utils::Hook(FindGetMaxTickRate(), GetMaxTickRate);
	if (VersionInfo.FortniteVersion >= 17)
		Utils::Hook(Memcury::Scanner::FindPattern("48 89 5C 24 10 48 89 6C 24 20 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B F9").Get(), CheckCheckpointHeartBeat);

	if (VersionInfo.EngineVersion < 4.20)
	{
		auto ApplyHomebaseEffectsOnPlayerSetupAddr = Memcury::Scanner::FindPattern("40 55 53 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 00 4C 8B").Get();

		Utils::Hook(ApplyHomebaseEffectsOnPlayerSetupAddr, ApplyHomebaseEffectsOnPlayerSetup, ApplyHomebaseEffectsOnPlayerSetupOG);
	}
}
