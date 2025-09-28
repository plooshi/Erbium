#include "pch.h"
#include "../Public/BuildingSMActor.h"

void ABuildingSMActor::Hook()
{
	auto OnDamageServerAddr = FindFunctionCall(L"OnDamageServer", VersionInfo.EngineVersion == 4.16 ? std::vector<uint8_t>{ 0x4C, 0x89, 0x4C } : VersionInfo.EngineVersion == 4.19 || VersionInfo.EngineVersion >= 4.27 ? std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 } : std::vector<uint8_t>{ 0x40, 0x55 });

	//Utils::Hook(OnDamageServerAddr, OnDamageServer);
}