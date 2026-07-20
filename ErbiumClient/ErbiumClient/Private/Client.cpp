#include "pch.h"
#include "../Public/Client.h"
#include "../../../Erbium/Erbium/Public/Configuration.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortPlaylistAthena.h"

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

        FUIExtension ArenaUIExtension{};
        ArenaUIExtension.Slot = PrimarySlot;
        if (VersionInfo.FortniteVersion < 23)
            ArenaUIExtension.WidgetClass.ObjectID.AssetPathName = FName(L"/Game/UI/Competitive/Arena/ArenaScoringHUD.ArenaScoringHUD_C");
        else
        {
            auto& PackageName = *(FName*)(__int64(&ArenaUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0x10 : 0x8));
            auto& AssetName = *(FName*)(__int64(&ArenaUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0x14 : 0xC));
            auto& SubPathString = *(FString*)(__int64(&ArenaUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0x18 : 0x10));

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
            auto& PackageName = *(FName*)(__int64(&ShowdownUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0x10 : 0x8));
            auto& AssetName = *(FName*)(__int64(&ShowdownUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0x14 : 0xC));
            auto& SubPathString = *(FString*)(__int64(&ShowdownUIExtension.WidgetClass) + (VersionInfo.EngineVersion < 5.3 ? 0x18 : 0x10));

            PackageName.ComparisonIndex = FName(L"/Game/UI/Frontend/Showdown/ShowdownScoringHUD").ComparisonIndex;
            AssetName.ComparisonIndex = FName(L"ShowdownScoringHUD_C").ComparisonIndex;
            SubPathString = FString();
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
                    Playlist->RespawnType = 1; // InfiniteRespawnExceptStorm
                    if (Playlist->HasbAllowJoinInProgress())
                        Playlist->bAllowJoinInProgress = true;
                    // if (Playlist->HasbForceRespawnLocationInsideOfVolume())
                    //	Playlist->bForceRespawnLocationInsideOfVolume = true;
                    if (Playlist->HasbForceCameraFadeOnRespawn())
                        Playlist->bForceCameraFadeOnRespawn = true;
                }
                if (VersionInfo.FortniteVersion >= 10)
                {
                    auto Name = Object->Name.ToString();
                    if (Name.contains("Showdown"))
                        Playlist->UIExtensions.Add(Name.contains("ShowdownAlt") ? ArenaUIExtension : ShowdownUIExtension);
                }
            }
        }
    }

    if (VersionInfo.FortniteVersion < 24.30)
    {
        auto EditModeInputComponent = Memcury::Scanner::FindStringRef<const wchar_t*>(L"EditModeInputComponent0");

        auto rdataSect = Memcury::PE::Section::GetSection(".rdata");
        for (int i = 1; i < 0x5000; i++)
        {
            auto Addr = (uint8_t*)EditModeInputComponent.Get() + i;

            if (*Addr == 0x48 && *(Addr + 1) == 0x8D)
            {
                auto Rel = Memcury::Scanner(Addr).RelativeOffset(3);

                if (rdataSect.isInSection(Rel.Get()) && strcmp((char*)Rel.Get(), "CompleteBuildingEditInteraction") == 0)
                {
                    CompleteBuildingEditInteraction = Memcury::Scanner(Addr).ScanFor({ 0x48, 0x8D }, false).RelativeOffset(3).GetAs<void (*)(void*)>();
                    break;
                }
            }
        }

        auto SelectEditScanner = EditModeInputComponent;
        SelectEditScanner.ScanFor({ 0x48, 0x8D, 0x05 }, true, 1);

        if (VersionInfo.FortniteVersion < 24.30)
        {
            auto SelectResetScanner = SelectEditScanner;
            auto SelectResetAddr = SelectResetScanner.ScanFor({ 0x48, 0x8D, 0x05 }, true).RelativeOffset(3).Get();

            Hooking::Hook(SelectResetAddr, SelectReset, SelectResetOG);
        }

        if (VersionInfo.FortniteVersion < 11)
        {
            auto SelectEditAddr = SelectEditScanner.RelativeOffset(3).Get();

            Hooking::Hook(SelectEditAddr, SelectEdit, SelectEditOG);
        }

        if (VersionInfo.FortniteVersion < 15.20)
        {
            auto rdataSect = Memcury::PE::Section::GetSection(".rdata");
            for (int i = 1; i < 0x5000; i++)
            {
                auto Addr = (uint8_t*)EditModeInputComponent.Get() - i;

                if (*Addr == 0x48 && *(Addr + 1) == 0x8D)
                {
                    auto Rel = Memcury::Scanner(Addr).RelativeOffset(3);

                    if (rdataSect.isInSection(Rel.Get()) && strcmp((char*)Rel.Get(), "PerformBuildingEditInteraction") == 0)
                    {
                        auto PerformBuildingEditInteractionAddr = Memcury::Scanner(Addr).ScanFor({ 0x48, 0x8D }, false).RelativeOffset(3).Get();

                        Hooking::Hook(PerformBuildingEditInteractionAddr, PerformBuildingEditInteraction, PerformBuildingEditInteractionOG);
                        break;
                    }
                }
            }
        }
        // MH_EnableHook(MH_ALL_HOOKS);
    }

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ClientThread, 0, 0, 0);
}
