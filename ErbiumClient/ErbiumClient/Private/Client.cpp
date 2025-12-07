#include "pch.h"
#include "../Public/FortPlaylistAthena.h"
#include "../Public/Configuration.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/Client.h"

bool bEOREnabled = false;
bool bDisablePreEdits = false;
inline void* (*SelectResetOG)(void*) = nullptr;
inline void* (*SelectEditOG)(void*) = nullptr;
inline void (*PerformBuildingEditInteractionOG)(void*) = nullptr;
inline void (*CompleteBuildingEditInteraction)(void*) = nullptr;

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
void PerformBuildingEditInteraction(AFortPlayerControllerAthena* _this)
{
	if (bDisablePreEdits && _this->TargetedBuilding->IsA<ABuildingPlayerPrimitivePreview>())
		return;

	return PerformBuildingEditInteractionOG(_this);
}

void ClientThread()
{
	bool bPressed = false;
	while (true)
	{
		if (UWorld::GetWorld() && UWorld::GetWorld()->OwningGameInstance)
		{
			auto& LocalPlayers = UWorld::GetWorld()->OwningGameInstance->LocalPlayers;

			if (LocalPlayers.Num() > 0)
			{
				auto PlayerController = (AFortPlayerControllerAthena*)LocalPlayers[0]->PlayerController;

				if (PlayerController && !PlayerController->CheatManager)
				{
					PlayerController->CheatManager = (UFortCheatManager*)UGameplayStatics::SpawnObject(PlayerController->CheatClass.Get(), PlayerController);
					PlayerController->CheatManager->ObjectFlags &= ~0x1000000;
					TUObjectArray::GetItemByIndex(PlayerController->CheatManager->Index)->Flags &= ~0x4000000;
				}
			}
		}

		if (!bPressed && GetAsyncKeyState(VK_F3))
		{
			bPressed = true;

			bEOREnabled ^= 1;
		}
		else if (!bPressed && GetAsyncKeyState(VK_F4))
		{
			bPressed = true;

			bDisablePreEdits ^= 1;
		}
		/*else if (!bPressed && GetAsyncKeyState(VK_F2))
		{
			bPressed = true;
			//bEnableResetOnRelease ^= 1;
		}*/
		else if (!GetAsyncKeyState(VK_F3) && !GetAsyncKeyState(VK_F4))
			bPressed = false;

		Sleep(33); // thread runs at 30tps	
	}
}

void Client::Init()
{
	UEngine::GetEngine()->GameViewport->ViewportConsole = UGameplayStatics::SpawnObject(UEngine::GetEngine()->ConsoleClass, UEngine::GetEngine()->GameViewport);

	if (VersionInfo.FortniteVersion >= 10 || FConfiguration::bForceRespawns)
	{
		auto PrimarySlot = uint8_t(EPlaylistUIExtensionSlot::StaticEnum() ? EPlaylistUIExtensionSlot::GetPrimary() : EUIExtensionSlot::GetPrimary());

		TArray<FUIExtension> ArenaExtensions, ShowdownExtensions;

		if (VersionInfo.FortniteVersion >= 10)
		{
			FUIExtension ArenaUIExtension{};
			ArenaUIExtension.Slot = PrimarySlot;
			if (VersionInfo.FortniteVersion < 23)
				ArenaUIExtension.WidgetClass.ObjectID.AssetPathName = FName(L"/Game/UI/Competitive/Arena/ArenaScoringHUD.ArenaScoringHUD_C");
			else
			{
				auto& PackageName = *(FName*)(__int64(&ArenaUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0xC : 0x8));
				auto& AssetName = *(FName*)(__int64(&ArenaUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0x10 : 0xC));
				auto& SubPathString = *(FString*)(__int64(&ArenaUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0x14 : 0x10));

				PackageName = FName(L"/Game/UI/Competitive/Arena/ArenaScoringHUD");
				AssetName = FName(L"ArenaScoringHUD_C");
				SubPathString = FString();
			}

			FUIExtension ShowdownUIExtension{};
			ShowdownUIExtension.Slot = PrimarySlot;
			if (VersionInfo.FortniteVersion < 23)
				ShowdownUIExtension.WidgetClass.ObjectID.AssetPathName = FName(L"/Game/UI/Frontend/Showdown/ShowdownScoringHUD.ShowdownScoringHUD_C");
			else
			{
				auto& PackageName = *(FName*)(__int64(&ShowdownUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0xC : 0x8));
				auto& AssetName = *(FName*)(__int64(&ShowdownUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0x10 : 0xC));
				auto& SubPathString = *(FString*)(__int64(&ShowdownUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0x14 : 0x10));

				PackageName = FName(L"/Game/UI/Frontend/Showdown/ShowdownScoringHUD");
				AssetName = FName(L"ShowdownScoringHUD_C");
				SubPathString = FString();
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

	if (VersionInfo.FortniteVersion < 24.30)
	{
		auto CompRef = Memcury::Scanner::FindStringRef(L"EditModeInputComponent0").Get();
		uintptr_t SelectEditAddr, SelectResetAddr, PerformBuildingEditInteractionAddr;

		int Skip = 0;
		for (int i = 1; i < 2000; i++)
		{
			if (*(uint8_t*)(CompRef + i) == 0x48 && *(uint8_t*)(CompRef + i + 1) == 0x8D && *(uint8_t*)(CompRef + i + 2) == 0x05)
			{
				if (Skip == 1)
					SelectEditAddr = Memcury::Scanner(CompRef + i).RelativeOffset(3).Get();
				else if (Skip == 2)
				{
					SelectResetAddr = Memcury::Scanner(CompRef + i).RelativeOffset(3).Get();
					break;
				}

				Skip++;
			}
		}
		
		auto rdataSect = Memcury::PE::Section::GetSection(".rdata");
		for (int i = 1; i < 0x5000; i++)
		{
			if ((*(uint8_t*)(CompRef - i) == 0x48 || *(uint8_t*)(CompRef - i) == 0x4C) && *(uint8_t*)(CompRef - i + 1) == 0x8D)
			{
				auto stringAddr = Memcury::Scanner(CompRef - i).RelativeOffset(3).Get();

				if (rdataSect.isInSection(stringAddr))
				{
					auto str = (char*)stringAddr;

					if (strcmp(str, "PerformBuildingEditInteraction") == 0)
					{
						for (int x = 1; x < 2000; x++)
						{
							if (*(uint8_t*)(CompRef - i - x) == 0x48 && *(uint8_t*)(CompRef - i - x + 1) == 0x8D && *(uint8_t*)(CompRef - i - x + 2) == 0x05)
							{
								PerformBuildingEditInteractionAddr = Memcury::Scanner(CompRef - i - x).RelativeOffset(3).Get();
								break;
							}
						}
						break;
					}
				}
			}
		}

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

		CompleteBuildingEditInteraction = (void (*)(void*)) Memcury::Scanner(CompleteBuildingEditInteractionLea).RelativeOffset(3).Get();

		MH_Initialize();

		if (VersionInfo.FortniteVersion < 11)
			Utils::Hook(SelectEditAddr, SelectEdit, SelectEditOG);
		if (VersionInfo.FortniteVersion < 15.20)
			Utils::Hook(PerformBuildingEditInteractionAddr, PerformBuildingEditInteraction, PerformBuildingEditInteractionOG);
		if (VersionInfo.FortniteVersion < 24.30)
			Utils::Hook(SelectResetAddr, SelectReset, SelectResetOG);

		MH_EnableHook(MH_ALL_HOOKS);
	}

	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ClientThread, 0, 0, 0);
}