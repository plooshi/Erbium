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
	return FConfiguration::MaxTickRate;
	//return std::clamp(1.f / DeltaTime, 1.f, FConfiguration::MaxTickRate);
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
					TUObjectArray::GetItemByIndex(PlayerController->CheatManager->Index)->Flags &= ~0x4000000;
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
	
	auto PrimarySlot = uint8_t(EPlaylistUIExtensionSlot::StaticEnum() ? EPlaylistUIExtensionSlot::GetPrimary() : EUIExtensionSlot::GetPrimary());

	if (VersionInfo.FortniteVersion >= 10 || FConfiguration::bForceRespawns)
	{
		TArray<FUIExtension> ArenaExtensions, ShowdownExtensions;

		if (VersionInfo.FortniteVersion >= 10)
		{
			FUIExtension ArenaUIExtension{};
			ArenaUIExtension.Slot = PrimarySlot;
			if (VersionInfo.FortniteVersion < 23)
				ArenaUIExtension.WidgetClass.ObjectID.AssetPathName = FName(L"/Game/UI/Competitive/Arena/ArenaScoringHUD.ArenaScoringHUD_C");
			else
			{
				auto& PackageName = *(FName*)(__int64(&ArenaUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.2 ? 0xC : 0x8));
				auto& AssetName = *(FName*)(__int64(&ArenaUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.2 ? 0x10 : 0xC));

				PackageName = FName(L"/Game/UI/Competitive/Arena/ArenaScoringHUD");
				AssetName = FName(L"ArenaScoringHUD_C");
			}

			FUIExtension ShowdownUIExtension{};
			ShowdownUIExtension.Slot = PrimarySlot;
			if (VersionInfo.FortniteVersion < 23)
				ShowdownUIExtension.WidgetClass.ObjectID.AssetPathName = FName(L"/Game/UI/Frontend/Showdown/ShowdownScoringHUD.ShowdownScoringHUD_C");
			else
			{
				auto& PackageName = *(FName*)(__int64(&ShowdownUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.2 ? 0xC : 0x8));
				auto& AssetName = *(FName*)(__int64(&ShowdownUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.2 ? 0x10 : 0xC));

				PackageName = FName(L"/Game/UI/Frontend/Showdown/ShowdownScoringHUD");
				AssetName = FName(L"ShowdownScoringHUD_C");
			}

			ArenaExtensions.Add(ArenaUIExtension);
			ShowdownExtensions.Add(ShowdownUIExtension);
		}

		auto PlaylistClass = FindClass("FortPlaylistAthena");

		for (int i = 0; i < TUObjectArray::Num(); i++)
		{
			auto Object = TUObjectArray::GetObjectByIndex(i);

			if (Object && Object->IsA((UClass*)PlaylistClass))
			{
				auto Playlist = (UFortPlaylistAthena*)Object;

				if (FConfiguration::bForceRespawns)
				{
					if (Playlist->HasbRespawnInAir())
						Playlist->bRespawnInAir = true;
					if (Playlist->HasRespawnHeight())
					{
						Playlist->RespawnHeight.Curve.CurveTable = nullptr;
						Playlist->RespawnHeight.Curve.RowName = FName();
						Playlist->RespawnHeight.Value = 20000;
					}
					if (Playlist->HasRespawnTime())
					{
						Playlist->RespawnTime.Curve.CurveTable = nullptr;
						Playlist->RespawnTime.Curve.RowName = FName();
						Playlist->RespawnTime.Value = 3;
					}
					Playlist->RespawnType = 1; // InfiniteRespawns
					if (Playlist->HasbAllowJoinInProgress())
						Playlist->bAllowJoinInProgress = true;
					if (Playlist->HasbForceRespawnLocationInsideOfVolume())
						Playlist->bForceRespawnLocationInsideOfVolume = true;
				}
				if (VersionInfo.FortniteVersion >= 10)
				{
					auto Name = Object->Name.ToString();
					if (Name.contains("Showdown"))
						Playlist->UIExtensions = Name.contains("ShowdownAlt") ? ArenaExtensions : ShowdownExtensions;
				}
			}
		}
	}

	if (VersionInfo.FortniteVersion < 20)
	{
		auto SelectEditAddr = Memcury::Scanner::FindStringRef(L"EditModeInputComponent0").ScanFor({ 0x48, 0x8D, 0x05 }, true, 1).RelativeOffset(3).Get();
		auto SelectResetAddr = Memcury::Scanner::FindStringRef(L"EditModeInputComponent0").ScanFor({ 0x48, 0x8D, 0x05 }, true, 2).RelativeOffset(3).Get();

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
			Utils::Hook(SelectEditAddr, SelectEdit, SelectEditOG);
		if (VersionInfo.FortniteVersion < 24.40)
			Utils::Hook(SelectResetAddr, SelectReset, SelectResetOG);

		MH_EnableHook(MH_ALL_HOOKS);
	}

	if (FConfiguration::bForceRespawns)
	{

	}

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ClientThread, 0, 0, 0);
}

class UIpNetDriver : public UNetDriver
{
public:
	UCLASS_COMMON_MEMBERS(UIpNetDriver);
};

void PatchAllNetModes(uintptr_t AttemptDeriveFromURL)
{
	Memcury::PE::Address add{ nullptr };

	const auto sizeOfImage = Memcury::PE::GetNTHeaders()->OptionalHeader.SizeOfImage;
	const auto scanBytes = reinterpret_cast<std::uint8_t*>(Memcury::PE::GetModuleBase());

	for (auto i = 0ul; i < sizeOfImage - 5; ++i)
	{
		if (scanBytes[i] == 0xE8 || scanBytes[i] == 0xE9)
		{
			if (Memcury::PE::Address(&scanBytes[i]).RelativeOffset(1).GetAs<void*>() == (void*)AttemptDeriveFromURL)
			{
				add = Memcury::PE::Address(&scanBytes[i]);

				// scan for the read of World->NetDriver

				for (auto j = 0; j > -0x100000; j--) // so we find everything. no func is actually 1mb
				{
					if ((scanBytes[i + j] & 0xF8) == 0x48 && ((scanBytes[i + j + 1] & 0xFC) == 0x80 || (scanBytes[i + j + 1] & 0xF8) == 0x38) && (scanBytes[i + j + 2] & 0xF0) != 0xC0 && (scanBytes[i + j + 2] & 0xF0) != 0xE0 && scanBytes[i + j + 2] != 0x65 && scanBytes[i + j + 2] != 0xBB && scanBytes[i + j + 3] == 0x38 && ((scanBytes[i + j + 1] & 0xFC) != 0x80 || scanBytes[i + j + 4] == 0x0))
					{
						// now, scan for if (NetDriver) return NM_Client;

						bool found = false;
						for (auto k = 4; k < 0x104; k++)
						{
							if (scanBytes[i + j + k] == 0x75)
							{
								auto Scuffness = __int64(&scanBytes[i + j + k + 5]);

								if (*(uint32_t*)Scuffness != 0xF0 && (scanBytes[i + j + k + 4] != 0xC || scanBytes[i + j + k + 5] != 0xB) && scanBytes[i + j + k + 4] != 0x09)
									continue;

								Utils::Patch<uint16_t>(__int64(&scanBytes[i + j + k]), 0x9090);
								if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
									Utils::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
								else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
								{
									DWORD og;
									VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
									*(uint32*)(&scanBytes[i + j]) = 0x90909090;
									*(uint8*)(&scanBytes[i + j + 4]) = 0x90;
									VirtualProtect(&scanBytes[i + j], 5, og, &og);
								}
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 2);
								found = true;
								break;
							}
							else if (scanBytes[i + j + k] == 0x74)	
							{
								auto Scuffness = __int64(&scanBytes[i + j + k]);
								Scuffness = (Scuffness + 2) + *(int8_t*)(Scuffness + 1);
								
								if (*(uint32_t*)(Scuffness + 3) != 0xF0 && (*(uint8_t*)(Scuffness + 2) != 0xC || *(uint8_t*)(Scuffness + 3) != 0xB) && *(uint8_t*)(Scuffness + 2) != 0x09)
									continue;

								Utils::Patch<uint8_t>(__int64(&scanBytes[i + j + k]), 0xeb);
								if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
									Utils::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
								else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
								{
									DWORD og;
									VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
									*(uint32*)(&scanBytes[i + j]) = 0x90909090;
									*(uint8*)(&scanBytes[i + j + 4]) = 0x90;
									VirtualProtect(&scanBytes[i + j], 5, og, &og);
								}
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 1);
								found = true;
								break;
							}
							else if (scanBytes[i + j + k] == 0x0F  && scanBytes[i + j + k + 1] == 0x85)
							{
								auto Scuffness = __int64(&scanBytes[i + j + k + 9]);

								if (*(uint32_t*)Scuffness != 0xF0 && (scanBytes[i + j + k + 8] != 0xC || scanBytes[i + j + k + 9] != 0xB) && scanBytes[i + j + k + 8] != 0x09)
									continue;

								DWORD og;
								VirtualProtect(&scanBytes[i + j + k], 6, PAGE_EXECUTE_READWRITE, &og);
								*(uint32*)(&scanBytes[i + j + k]) = 0x90909090;
								*(uint16*)(&scanBytes[i + j + k + 4]) = 0x9090;
								VirtualProtect(&scanBytes[i + j + k], 6, og, &og);
								if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
									Utils::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
								else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
								{
									DWORD og;
									VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
									*(uint32*)(&scanBytes[i + j]) = 0x90909090;
									*(uint8*)(&scanBytes[i + j + 4]) = 0x90;
									VirtualProtect(&scanBytes[i + j], 5, og, &og);
								}
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 6);
								found = true;
								break;
							}
							else if (scanBytes[i + j + k] == 0x0F && scanBytes[i + j + k + 1] == 0x84)
							{
								auto Scuffness = __int64(&scanBytes[i + j + k]);
								Scuffness = (Scuffness + 6) + *(int32_t*)(Scuffness + 2);

								if (*(uint32_t*)(Scuffness + 3) != 0xF0 && (*(uint8_t*)(Scuffness + 2) != 0xC || *(uint8_t*)(Scuffness + 3) != 0xB) && *(uint8_t*)(Scuffness + 2) != 0x09)
									continue;

								Utils::Patch<uint16_t>(__int64(&scanBytes[i + j + k]), 0xe990);
								if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
									Utils::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
								else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
								{
									DWORD og;
									VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
									*(uint32*)(&scanBytes[i + j]) = 0x90909090;
									*(uint8*)(&scanBytes[i + j + 4]) = 0x90;
									VirtualProtect(&scanBytes[i + j], 5, og, &og);
								}
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 2);
								found = true;
								break;
							}
						}
						if (found)
							break;
					}
				}
			}
		}
	}
}

bool RetFalse()
{
	return false;
}

class AFortTeamMemberPedestal : public AActor
{
public:
	UCLASS_COMMON_MEMBERS(AFortTeamMemberPedestal);
};

void Misc::Hook()
{
	if (VersionInfo.FortniteVersion == 23.00 || (VersionInfo.FortniteVersion >= 25 && VersionInfo.FortniteVersion != 28.30 && VersionInfo.FortniteVersion != 29.40) || VersionInfo.FortniteVersion >= 30)
	{
		auto AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B C1").Get();
		if (!AttemptDeriveFromURL)
			AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8B D1").Get();
		if (!AttemptDeriveFromURL)
			AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B D1").Get();

		Utils::Hook(AttemptDeriveFromURL, GetNetMode);
		PatchAllNetModes(AttemptDeriveFromURL);
	}
	else if (VersionInfo.FortniteVersion >= 28)
	{
		auto AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B C1").Get();
		if (!AttemptDeriveFromURL)
			AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8B D1").Get();
		if (!AttemptDeriveFromURL)
			AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B D1").Get();

		Utils::Hook(AttemptDeriveFromURL, GetNetMode);
		PatchAllNetModes(AttemptDeriveFromURL);

		Utils::Hook(FindGetNetMode(), GetNetMode);
	}
	else
		Utils::Hook(FindGetNetMode(), GetNetMode);

	Utils::Hook(FindSendRequestNow(), SendRequestNow, SendRequestNowOG);
	Utils::Hook(FindGetMaxTickRate(), GetMaxTickRate);
	if (VersionInfo.FortniteVersion >= 17)
	{
		auto pattern = Memcury::Scanner::FindPattern("48 89 5C 24 10 48 89 6C 24 20 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B F9").Get();

		if (!pattern)
			pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B E9").Get();

		if (!pattern)
			pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 55 41 56 48 81 EC ? ? ? ? 65 48 8B 04 25").Get();

		Utils::Hook(pattern, CheckCheckpointHeartBeat);
	}
	if (VersionInfo.EngineVersion < 4.20)
	{
		auto ApplyHomebaseEffectsOnPlayerSetupAddr = Memcury::Scanner::FindPattern("40 55 53 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 00 4C 8B").Get();

		Utils::Hook(ApplyHomebaseEffectsOnPlayerSetupAddr, ApplyHomebaseEffectsOnPlayerSetup, ApplyHomebaseEffectsOnPlayerSetupOG);
	}
	if (VersionInfo.FortniteVersion >= 25 && VersionInfo.FortniteVersion < 28)
	{
		Utils::Hook(Memcury::Scanner::FindPattern("48 89 5C ? ? 57 48 83 EC ? 48 8B D1 48 85 C9 74 ?").Get(), RetFalse);
	}


	auto PedestalBeginPlay = Memcury::Scanner::FindStringRef(L"AFortTeamMemberPedestal::BeginPlay - Begun play on pedestal %s", true, 0, VersionInfo.EngineVersion >= 5.0).Get();

	if (PedestalBeginPlay)
	{
		uint64_t RealBeginPlay = 0;
		for (int i = 0; i < 1000; i++)
		{
			auto Ptr = (uint8_t*)(PedestalBeginPlay - i);

			if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c)
			{
				RealBeginPlay = (uint64_t)Ptr;
				break;
			}
			else if (*Ptr == 0x40 && *(Ptr + 1) == 0x53 && *(Ptr + 2) == 0x41 && *(Ptr + 3) == 0x56)
			{
				RealBeginPlay = (uint64_t)Ptr;
				break;
			}
		}

		auto ActorVft = AFortTeamMemberPedestal::GetDefaultObj()->Vft;

		for (int i = 0; i < 0x500; i++)
		{
			if (ActorVft[i] == (void*)RealBeginPlay)
			{
				Utils::Hook<AFortTeamMemberPedestal>(uint32_t(i), AActor::GetDefaultObj()->Vft[i]);
				break;
			}
		}
	}
	
	if (VersionInfo.FortniteVersion >= 23)
	{
		auto pattern = Memcury::Scanner::FindPattern("48 8B 01 FF 90 ? ? ? ? 48 8B 8B ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 90 ? ? ? ? 48 8D 8B");

		auto patchPoint = pattern.ScanFor(VersionInfo.EngineVersion < 5.5 ? std::vector<uint8_t>{ 0x48, 0x89, 0x5C } : std::vector<uint8_t>{ 0x40, 0x53 }, false).ScanFor({0x83, 0xF8, 0x02}).Get();

		Utils::Patch<uint8_t>(patchPoint + 2, 0x1);
	}
}
