#include "pch.h"
#include "../Public/Misc.h"
#include "../../Erbium/Public/Finders.h"
#include <algorithm>
#include "../../Erbium/Public/Configuration.h"

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
	if (VersionInfo.FortniteVersion == 1.72 || VersionInfo.FortniteVersion == 1.8)
	{
		static auto ItemDefOffset = a5->GetOffset("ItemDefinition");
		GetFromOffset<const UObject*>(a5, ItemDefOffset) = Utils::FindObject<UObject>(L"/Game/Athena/Heroes/HID_Commando_Athena_Menu.HID_Commando_Athena_Menu");
	}

	return ApplyHomebaseEffectsOnPlayerSetupOG(a1, a2, a3, a4, a5, a6, a7);
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