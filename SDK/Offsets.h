#pragma once
#include "Memcury.h"
#include <array>

namespace SDK
{
	struct FVersionInfo
	{
		double EngineVersion = 0.f;
		double FortniteVersion = 0.f;
	};
	struct FStringNoOps
	{
		wchar_t* Data;
		int32_t NumElements;
		int32_t MaxElements;
	};
	inline FVersionInfo VersionInfo{};

	namespace Offsets
	{
		inline uint64_t Realloc = 0;
		inline uint64_t AppendString = 0;
		inline uint64_t ToString = 0;
		inline uint64_t ProcessEventVft = 0;
		inline uint64_t GObjectsChunked = 0;
		inline uint64_t GObjectsUnchunked = 0;
		inline uint64_t Step = 0;
		inline uint64_t StepExplicitProperty = 0;
		inline uint64_t GetInterfaceAddress = 0;
		inline uint64_t StaticFindObject = 0;
		inline uint64_t StaticLoadObject = 0;
		inline uint64_t FNameConstructor = 0;
		inline uint64_t SpawnActor = 0;

		inline uint32_t Offset_Internal = 0;
		inline uint32_t ElementSize = 0;
		inline uint32_t PropertyFlags = 0;
		inline uint32_t PropertiesSize = 0;
		inline uint32_t Super = 0;
		inline uint32_t FieldMask = 0;
		inline uint32_t Children = 0;
		inline uint32_t FField_Next = 0;
		inline uint32_t FField_Name = 0;
		inline uint32_t ExecFunction = 0;
		inline uint32_t FFrame_PropertyChainForCompiledIn = 0;
		inline uint32_t FFrame_CurrentNativeFunction = 0;
		inline uint32_t FFrame_Next = 0;
	}

	extern void UpdateNumElemsPerChunk();
	extern void InitializeProcessEventVft(uintptr_t);

	inline void Init()
	{
		FStringNoOps OutVar;

		auto GetEngineVersionMethod1 = Memcury::Scanner::FindPattern("40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 8B C8 41 B8 04 ? ? ? 48 8B D3");
		if (!GetEngineVersionMethod1.Get())
			GetEngineVersionMethod1 = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC ? 65 48 8B 04 25 ? ? ? ? 48 8B D9 B9 ? ? ? ? 48 8B 10 8B 04 11 39 05 ? ? ? ? 7E ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 83 3D ? ? ? ? ? 75 ? 48 8D 3D ? ? ? ? 48 8B CF E8 ? ? ? ? 48 8D 0D ? ? ? ? 48 89 3D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B 0D ? ? ? ? 41 B8");

		if (auto GetEngineVersion = (FStringNoOps * (*)(FStringNoOps * _Out)) GetEngineVersionMethod1.Get())
		{
			GetEngineVersion(&OutVar);
		}
		else
		{
			auto GetEngineVersionMethod2 = Memcury::Scanner::FindPattern("40 53 48 83 EC ? 33 C0 49 8B D8 48 39 42 ? 0F 95 C0 48 01 42 ? E8 ? ? ? ? 41 B8");
			auto CopyOfMethod2 = GetEngineVersionMethod2;

			auto GetStorage = (void* (*)()) GetEngineVersionMethod2.RelativeOffset(23).Get();
			auto GetEngineVersion2 = (void (*)(void*, FStringNoOps*, int)) CopyOfMethod2.RelativeOffset(42).Get();

			GetEngineVersion2(GetStorage(), &OutVar, 4); // no idea why 4 but sure
		}

		std::wstring BuildString = OutVar.Data;
		std::wstring EngineVersion = BuildString.substr(0, BuildString.find(L'-'));
		std::wstring FortniteCL = BuildString.substr(BuildString.find(L'-') + 1, BuildString.find(L'+') - BuildString.find(L'-') - 1);

		if (EngineVersion == L"4.26.1")
			EngineVersion = L"4.27.0";

		if (EngineVersion.find_first_of(L'.') != EngineVersion.find_last_of(L'.'))
			EngineVersion.erase(EngineVersion.rfind(L'.'));

		auto FortniteCLNum = std::stoull(FortniteCL);

		VersionInfo.EngineVersion = std::stod(EngineVersion);
		// these builds were just called "Cert"
		if (FortniteCLNum < 3901517)
		{
			switch (FortniteCLNum)
			{
			case 3668626:
				VersionInfo.FortniteVersion = 1.64;
				break;
			case 3681159:
				VersionInfo.FortniteVersion = 1.7;
				break;
			case 3700114:
				VersionInfo.FortniteVersion = 1.72;
				break;
			case 3724489:
				VersionInfo.FortniteVersion = 1.8;
				break;
			case 3729133:
				VersionInfo.FortniteVersion = 1.81;
				break;
			case 3741772:
				VersionInfo.FortniteVersion = 1.82;
				break;
			case 3757339:
				VersionInfo.FortniteVersion = 1.9;
				break;
			case 3775276:
				VersionInfo.FortniteVersion = 1.91;
				break;
			case 3790078:
				VersionInfo.FortniteVersion = 1.10;
				break;
			case 3807424:
				VersionInfo.FortniteVersion = 1.11;
				break;
			case 3825894:
				VersionInfo.FortniteVersion = 2.1;
				break;
			case 3841827:
				VersionInfo.FortniteVersion = 2.2;
				break;
			case 3847564:
				VersionInfo.FortniteVersion = 2.3;
				break;
			case 3858292:
				VersionInfo.FortniteVersion = 2.4;
				break;
			case 3870737:
				VersionInfo.FortniteVersion = 2.42;
				break;
			case 3889387:
				VersionInfo.FortniteVersion = 2.5;
				break;
			}
		}
		else 
			VersionInfo.FortniteVersion = std::stod(BuildString.substr(BuildString.rfind(L'-') + 1));

		bUE51 = VersionInfo.FortniteVersion >= 24.00;

		Offsets::Realloc = Memcury::Scanner::FindPattern("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 48 8B F1 41 8B D8 48 8B 0D ? ? ? ?").Get();

		auto SRef = Memcury::Scanner::FindStringRef("ForwardShadingQuality_");
		constexpr std::array<const char*, 5> sigs =
		{
			"48 8D ? ? 48 8D ? ? E8",
			"48 8D ? ? ? 48 8D ? ? E8",
			"48 8D ? ? 49 8B ? E8",
			"48 8D ? ? ? 49 8B ? E8",
			"48 8D ? ? 48 8B ? E8"
		};

		for (auto& sig : sigs)
		{
			auto Scanner = SRef;
			Scanner.ScanFor(sig, true, 0, 1, VersionInfo.EngineVersion == 5.0 ? 0x100 : 0x50);

			if (Scanner.Get() != SRef.Get())
			{
				auto p2b = Memcury::ASM::pattern2bytes(sig);

				Offsets::AppendString = Scanner.RelativeOffset((uint32_t)p2b.size()).Get();
				break;
			}
		}

		if (!Offsets::AppendString || VersionInfo.EngineVersion > 5.3) // i dk what ver they inlined it on
		{
			Offsets::ToString = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 45 33 ED 48 8B FA 4C 89 2A").Get();

			if (!Offsets::ToString)
				Offsets::ToString = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 33 ED 48 8B FA 48 89 2A 48 89 6A ? 8B 19").Get();
		}

		uintptr_t addr = 0;

		if (VersionInfo.FortniteVersion < 14.00)
			addr = Memcury::Scanner::FindStringRef(L"AccessNoneNoContext").ScanFor({ 0x40, 0x55 }, true, 0, 1, 2000).Get();
		else if (floor(VersionInfo.FortniteVersion) == 27)
			addr = Memcury::Scanner::FindPattern("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 45 33 E4 4C 89 45 ? 4D 8B F8").Get();
		else if (VersionInfo.EngineVersion == 5.2 || (std::floor(VersionInfo.FortniteVersion) == 24 && VersionInfo.FortniteVersion >= 24.30))
			addr = Memcury::Scanner::FindPattern("40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C5 48 89 85 ? ? ? ? 45 33 F6").Get();
		else if (VersionInfo.FortniteVersion >= 23.00)
		{
			addr = Memcury::Scanner::FindPattern("48 85 C9 0F 85 ? ? ? ? F7 87 ? ? ? ? ? ? ? ? ? 8B ?").ScanFor({ 0x40, 0x55 }, false).Get();
			if (!addr)
				addr = Memcury::Scanner::FindPattern("41 FF 92 ? ? ? ? E9 ? ? ? ? 49 8B C8").ScanFor({ 0x40, 0x55 }, false).Get();
		}
		else
			addr = Memcury::Scanner::FindStringRef(L"UMeshNetworkComponent::ProcessEvent: Invalid mesh network node type: %s", true, 0, VersionInfo.FortniteVersion >= 19.00).ScanFor({ 0xE8 }, true, VersionInfo.FortniteVersion < 19.00 ? 1 : 3, VersionInfo.FortniteVersion == 15.50 ? 7 : 0, 2000).RelativeOffset(1).Get();

		if (VersionInfo.EngineVersion >= 4.21)
		{
			if (VersionInfo.FortniteVersion <= 6.01)
				UpdateNumElemsPerChunk();

			Offsets::GObjectsChunked = Memcury::Scanner::FindPattern(VersionInfo.FortniteVersion <= 6.02 ? "48 8B 05 ? ? ? ? 48 8B 0C C8 48 8D 04 D1" : "48 8B 05 ? ? ? ? 48 8B 0C C8 48 8B 04 D1").RelativeOffset(3).Get();
		}
		else
		{
			auto Addr = Memcury::Scanner::FindPattern("48 8B 05 ? ? ? ? 48 8D 14 C8 EB 03 49 8B D6 8B 42 08 C1 E8 1D A8 01 0F 85 ? ? ? ? F7 86 ? ? ? ? ? ? ? ?", false);
			if (!Addr.Get())
				Addr = Memcury::Scanner::FindPattern("48 8B 05 ? ? ? ? 48 8D 1C C8 81 4B ? ? ? ? ? 49 63 76 30", false);

			Offsets::GObjectsUnchunked = Addr.RelativeOffset(3).Get();
		}

		Offsets::Step = Memcury::Scanner::FindPattern("48 8B 41 20 4C 8B D2 48 8B D1 44 0F B6 08 48 FF").Get();
		if (!Offsets::Step)
			Offsets::Step = Memcury::Scanner::FindPattern("48 8B 41 ? 4C 8B DA 44 0F B6 08").Get();

		if (VersionInfo.EngineVersion >= 5.4 || VersionInfo.EngineVersion == 5.2)
			Offsets::StepExplicitProperty = Memcury::Scanner::FindPattern("41 8B 40 ? 4D 8B C8 48 0F BA E0").Get();
		else if (VersionInfo.EngineVersion == 5.3)
			Offsets::StepExplicitProperty = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 54 41 56 41 57 48 83 EC ? 41 8B 40 ? 49 8B D8 48 8B F2").Get();
		else if (VersionInfo.FortniteVersion >= 20.20)
			Offsets::StepExplicitProperty = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 41 8B 40 ? 49 8B D8 48 8B F2").Get();
		else
			Offsets::StepExplicitProperty = Memcury::Scanner::FindPattern("41 8B 40 ? 4D 8B C8").Get();

		if (VersionInfo.EngineVersion <= 4.21)
			Offsets::GetInterfaceAddress = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 33 FF 48 8B DA 48 8B F1 48").Get();
		else
		{
			Offsets::GetInterfaceAddress = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 33 DB 48 8B FA 48 8B F1 48 85 D2 0F 84 ? ? ? ? 8B 82 ? ? ? ? C1 E8").Get();

			if (!Offsets::GetInterfaceAddress)
			{
				Offsets::GetInterfaceAddress = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 33 DB 48 8B FA 48 8B F1 48 85 D2 74 ? F7 82").Get();

				if (!Offsets::GetInterfaceAddress)
				{
					Offsets::GetInterfaceAddress = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 33 DB 48 8B FA 48 8B F1 48 85 D2 0F 84 ? ? ? ? F7 82").Get();

					if (!Offsets::GetInterfaceAddress)
						Offsets::GetInterfaceAddress = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 83 EC ? 33 DB 48 8B FA 48 8B E9").Get();

					if (!Offsets::GetInterfaceAddress)
						Offsets::GetInterfaceAddress = Memcury::Scanner::FindPattern("4C 8B DC 49 89 5B ? 49 89 73 ? 57 48 83 EC ? 33 DB 48 8B FA 48 8B F1").Get();

					if (!Offsets::GetInterfaceAddress)
						Offsets::GetInterfaceAddress = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 33 DB 48 8B FA").Get();
				}
			}
		}

		if (VersionInfo.EngineVersion >= 5.3)
		{
			Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC ? 4C 8B E9 48 8D 4D").Get();

			if (!Offsets::StaticFindObject)
				Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC ? 4C 8B E9").Get();

			if (!Offsets::StaticFindObject)
				Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 83 FA FF").Get();
		}
		else if (VersionInfo.EngineVersion == 5.2)
			Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 56 41 57 48 8B EC 48 83 EC ? 33 DB 4C 8B F9").Get();
		else if (VersionInfo.EngineVersion >= 5.1)
		{
			Offsets::StaticFindObject = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 33 F6 4C 8B E1 48 83 CB", false).Get();

			if (!Offsets::StaticFindObject)
				Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8B EC 48 83 EC 60 33 DB 4C 8B F1 48 8D 4D E8 41 8A F1", false).Get();

			if (!Offsets::StaticFindObject)
				Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 56 41 57 48 8B EC 48 83 EC 60 33 DB 4C 8B F9 48 8D 4D E8 45").Get();
		}
		else if (VersionInfo.EngineVersion == 5.0)
		{
			Offsets::StaticFindObject = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 45 33 F6 4C 8B E1 45 0F B6 E9 49 8B F8 41 8B C6", false).Get();

			if (!Offsets::StaticFindObject)
			{
				Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 4C 89 64 24 ? 55 41 55 41 57 48 8B EC 48 83 EC 60 45 8A E1 4C 8B E9 48 83 FA").Get();

				if (!Offsets::StaticFindObject)
				{
					Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 4C 89 64 24 ? 55 41 55 41 57 48 8B EC 48 83 EC 50 4C 8B E9").Get();

					if (!Offsets::StaticFindObject)
						Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 7C 24 ? 4C 89 64 24 ? 55 41 56 41 57 48 8B EC 48 83 EC 60 33 FF 4C 8B E1 48 8D 4D E8 45 8A").Get();
				}
			}
		}
		else if (VersionInfo.EngineVersion >= 4.27)
		{
			if (floor(VersionInfo.FortniteVersion) == 18)
				Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 45 33 ED 45 8A F9 44 38 2D ? ? ? ? 49 8B F8 48 8B").Get();
			else if (VersionInfo.FortniteVersion == 16.50)
				Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 45 33 ED 45 8A F9 44 38 2D ? ? ? ? 49 8B F8 48 8B F2 4C 8B E1").Get();
			
			if (!Offsets::StaticFindObject)
				Offsets::StaticFindObject = Memcury::Scanner::FindPattern("40 55 53 57 41 54 41 55 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85").Get();
		}
		else if (VersionInfo.EngineVersion == 4.16)
			Offsets::StaticFindObject = Memcury::Scanner::FindPattern("4C 8B DC 57 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 49 89 6B F0 49 89 73 E8").Get();
		else if (VersionInfo.EngineVersion == 4.19)
		{
			Offsets::StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8B EC 48 83 EC 60 80 3D ? ? ? ? ? 45 0F B6 F1 49 8B F8").Get();

			if (!Offsets::StaticFindObject)
				Offsets::StaticFindObject = Memcury::Scanner::FindPattern("4C 8B DC 49 89 5B 08 49 89 6B 18 49 89 73 20 57 41 56 41 57 48 83 EC 60 80 3D").Get();
		}
		else
		{
			auto sRef = Memcury::Scanner::FindStringRef(L"Illegal call to StaticFindObject() while serializing object data!", false, 1).Get();

			for (int i = 0; i < 1000; i++)
			{
				auto Ptr = (uint8_t*)(sRef - i);

				if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
				{
					Offsets::StaticFindObject = uint64_t(Ptr);
					break;
				}
			}
		}

		if (VersionInfo.EngineVersion >= 5.4)
		{
			Offsets::StaticLoadObject = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 8B 85 ? ? ? ? 33 FF 8B 35").Get();

			if (!Offsets::StaticLoadObject)
				Offsets::StaticLoadObject = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 8B 85").Get();

			if (!Offsets::StaticLoadObject)
				Offsets::StaticLoadObject = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B 85 ? ? ? ? 33 FF 48 8B B5").Get();

			if (!Offsets::StaticLoadObject)
				Offsets::StaticLoadObject = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B 85 ? ? ? ? 45 33 FF 4C 8B B5 ? ? ? ? 49 8B D8").Get();

			if (!Offsets::StaticLoadObject)
				Offsets::StaticLoadObject = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B 85 ? ? ? ? 45 33 E4 4C 8B B5 ? ? ? ? 49 8B D8").Get();
		}
		else
		{
			auto sRef = Memcury::Scanner::FindStringRef(L"STAT_LoadObject", false).Get();

			if (!sRef)
			{
				auto sRef2 = Memcury::Scanner::FindStringRef(L"Calling StaticLoadObject during PostLoad may result in hitches during streaming.");

				if (!sRef2.Get())
					sRef2 = Memcury::Scanner::FindStringRef(L"Calling StaticLoadObject(\"%s\", \"%s\", \"%s\") during PostLoad of %s is illegal and will crash in a cooked runtime", 0, false, VersionInfo.FortniteVersion >= 19);

				Offsets::StaticLoadObject = sRef2.ScanFor({ 0x40, 0x55 }, false).Get();
			}
			else
			{
				for (int i = 0; i < 400; i++)
				{
					if (*(uint8_t*)(sRef - i) == 0x4C && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x4C)
					{
						Offsets::StaticLoadObject = sRef - i;
						break;
					}
					else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
					{
						Offsets::StaticLoadObject = sRef - i;
						break;
					}
				}
			}
		}

		Offsets::Offset_Internal = VersionInfo.FortniteVersion >= 12.10 && VersionInfo.FortniteVersion < 20 ? 0x4c : (VersionInfo.FortniteVersion >= 24.30 ? 0x3c : 0x44);
		Offsets::PropertyFlags = Offsets::Offset_Internal - 0xc;
		Offsets::ElementSize = Offsets::Offset_Internal - 0x10;
		Offsets::PropertiesSize = VersionInfo.FortniteVersion >= 12.10 ? 0x58 : (VersionInfo.EngineVersion >= 4.22 ? 0x50 : 0x40);
		Offsets::Super = VersionInfo.EngineVersion >= 4.22 ? 0x40 : 0x30;
		Offsets::FieldMask = VersionInfo.FortniteVersion >= 24.30 ? 0x6b : (VersionInfo.FortniteVersion >= 12.10 && VersionInfo.FortniteVersion < 20 ? 0x7b : 0x73);
		Offsets::Children = VersionInfo.EngineVersion >= 4.22 ? 0x48 : 0x38;
		Offsets::FField_Next = VersionInfo.FortniteVersion >= 24.30 ? 0x18 : 0x20;
		Offsets::FField_Name = VersionInfo.FortniteVersion >= 24.30 ? 0x20 : 0x28;
		Offsets::FFrame_PropertyChainForCompiledIn = VersionInfo.FortniteVersion >= 20.20 ? 0x88 : 0x80;
		Offsets::FFrame_CurrentNativeFunction = VersionInfo.FortniteVersion >= 20.20 ? 0x90 : 0x88;
		Offsets::FFrame_Next = VersionInfo.FortniteVersion >= 24.30 ? 0x18 : (VersionInfo.FortniteVersion >= 12.10 ? 0x20 : 0x28);

		if (VersionInfo.EngineVersion < 4.22)
			Offsets::ExecFunction = 0xB0;
		else if (VersionInfo.EngineVersion >= 4.22 && VersionInfo.EngineVersion < 4.25)
			Offsets::ExecFunction = 0xC0;
		else if (VersionInfo.FortniteVersion >= 12.00 && VersionInfo.FortniteVersion < 12.10)
			Offsets::ExecFunction = 0xC8;
		else if (VersionInfo.FortniteVersion >= 12.10 && VersionInfo.FortniteVersion <= 12.61)
			Offsets::ExecFunction = 0xF0;
		else
			Offsets::ExecFunction = 0xD8;


		auto StringRef = Memcury::Scanner::FindStringRef(L"ClientIgnoreLookInput", true).Get();

		if (StringRef)
		{
			for (int i = 0; i < 1000; i++)
			{
				auto Ptr = (uint8_t*)(StringRef + i);

				if (*Ptr == 0x48 && *(Ptr + 1) == 0x8D && (*(Ptr + 7) == 0xE9 || *(Ptr + 7) == 0xE8))
				{
					Offsets::FNameConstructor = Memcury::Scanner(Ptr + 7).RelativeOffset(1).Get();
					break;
				}
			}
		}

		if (VersionInfo.EngineVersion >= 4.27)
		{
			auto stat = Memcury::Scanner::FindStringRef(L"STAT_SpawnActorTime").Get();

			for (int i = 0; i < 0x1000; i++)
			{
				if (*(uint8_t*)(stat - i) == 0x40 && *(uint8_t*)(stat - i + 1) == 0x55)
				{
					Offsets::SpawnActor = stat - i;
					break;
				}
				else if (*(uint8_t*)(stat - i) == 0x48 && *(uint8_t*)(stat - i + 1) == 0x8B && *(uint8_t*)(stat - i + 2) == 0xC4)
				{
					Offsets::SpawnActor = stat - i;
					break;
				}
			}
		}
		else
		{
			auto sRef = Memcury::Scanner::FindStringRef(L"SpawnActor failed because no class was specified");

			if (VersionInfo.FortniteVersion <= 3.3)
				Offsets::SpawnActor = sRef.ScanFor({ 0x40, 0x55 }, false, 0, 1, 3000).Get();
			else 
				Offsets::SpawnActor = sRef.ScanFor({ 0x4C, 0x8B, 0xDC }, false, 0, 1, 3000).Get();
		}

		InitializeProcessEventVft(addr);
	}
}
