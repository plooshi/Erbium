// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "../Public/Utils.h"
#include <thread>
#include <iostream>
#include "../Public/Finders.h"
#include "../../FortniteGame/Public/FortInventory.h"
#include <chrono>
#include "../Public/Configuration.h"
#include "../Public/Misc.h"
#include "../Public/GUI.h"
#include "../../Erbium/Plugins/CrashReporter/Public/CrashReporter.h"
#include "../../FortniteGame/Public/FortPlayerControllerAthena.h"
#include "../../Engine/Public/NetDriver.h"
#pragma comment(lib, "libcurl/libcurl.lib")
#pragma comment(lib, "libcurl/zlib.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Normaliz.lib")

void Main()
{
    if constexpr (!FConfiguration::bGUI)
        AllocConsole();

    if constexpr (!FConfiguration::bGUI || !FConfiguration::bUseStdoutLog)
    {
        if (!FConfiguration::bGUI || GetConsoleWindow())
        {
            FILE* s;
            freopen_s(&s, "CONOUT$", "w", stdout);
            freopen_s(&s, "CONOUT$", "w+", stderr);
            freopen_s(&s, "CONIN$", "r", stdin);
        }
    }

    if constexpr (FConfiguration::bCustomCrashReporter)
        FCrashReporter::Register();

    printf("Initializing SDK...\n");
    SDK::Init();

    if constexpr (FConfiguration::bGUI)
    {
        if constexpr (FConfiguration::bUseStdoutLog)
        {
            FILE* s;
            freopen_s(&s, "stdout.log", "w", stdout);
            freopen_s(&s, "stdout.log", "w+", stderr);
        }

        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)GUI::Init, 0, 0, 0);
    }

    if (VersionInfo.EngineVersion >= 5.0)
    {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"log LogFortUIDirector None"), nullptr);
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"log LogFortUIManager None"), nullptr);
    }
    if (VersionInfo.EngineVersion >= 5.1)
    {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"net.AllowEncryption 0"), nullptr);

        auto DefaultCurieGlobals = FindClass("CurieGlobals")->GetDefaultObj();

        if (DefaultCurieGlobals)
        {
            uint32 Offset = DefaultCurieGlobals->GetOffset("bEnableCurie");

            //if (Offset != -1)
            //    *(bool*)(uintptr_t(DefaultCurieGlobals) + Offset) = false;
        }
    }
    if (VersionInfo.EngineVersion >= 5.3 && FConfiguration::bEnableIris)
    {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"log LogIris None"), nullptr);
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"log LogIrisRpc None"), nullptr);
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"log LogIrisBridge None"), nullptr);
        /*auto IrisBool = Memcury::Scanner::FindPattern("83 3D ? ? ? ? ? 0F 8E ? ? ? ? 49 8B B9").RelativeOffset(2, 1).Get();
        if (IrisBool)
            *(uint32_t*)IrisBool = true;
        else
        {
            IrisBool = Memcury::Scanner::FindPattern("44 39 25 ? ? ? ? 0F 9F C0 45 84 FF").RelativeOffset(3).Get();

            if (IrisBool)
                *(uint32_t*)IrisBool = true;
        }*/
        auto IrisBool = FindCVar<uint32_t>(L"net.Iris.UseIrisReplication");

        if (IrisBool)
            *IrisBool = true;

        if (VersionInfo.FortniteVersion >= 29)
        {
            auto ReplicationBridgeConfig = UObjectReplicationBridgeConfig::GetDefaultObj();

            auto FortInventoryName = FName(L"/Script/FortniteGame.FortInventory");
            for (int i = 0; i < ReplicationBridgeConfig->FilterConfigs.Num(); i++)
            {
                auto& FilterConfig = ReplicationBridgeConfig->FilterConfigs.Get(i, FObjectReplicationBridgeFilterConfig::Size());

                if (FilterConfig.ClassName == FortInventoryName)
                {
                    FilterConfig.DynamicFilterName = FName(0);
                    break;
                }
            }
        }
        //UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"net.Iris.UseIrisReplication 1"), nullptr);
    }
    if (VersionInfo.EngineVersion >= 5.4)
    {
        // sprint fix
        auto SprintCVar = FindCVar<uint32_t>(L"Fort.MME.TacticalSprint");
        auto HurdleCVar = FindCVar<uint32_t>(L"Fort.MME.Hurdle");
        auto SlideCVar = FindCVar<uint32_t>(L"Fort.MME.Sliding");
        auto MantleCVar = FindCVar<uint32_t>(L"Fort.MME.Clambering");

        //if (SprintCVar)
        //    *SprintCVar = false;

        if (HurdleCVar)
            *HurdleCVar = false;

        if (SlideCVar)
            *SlideCVar = false;

        if (MantleCVar)
            *MantleCVar = false;
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"Fort.MME.TacticalSprint 0"), nullptr);
        /*UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"Fort.MME.Hurdle 0"), nullptr);
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"Fort.MME.Sliding 0"), nullptr);
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"Fort.MME.Clambering 0"), nullptr);*/
    }

#ifdef CLIENT
    Misc::InitClient();

    return;
#endif

    if constexpr (FConfiguration::WebhookURL && *FConfiguration::WebhookURL)
        curl_global_init(CURL_GLOBAL_ALL);

    sprintf_s(GUI::windowTitle, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Setting up" : (VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "Erbium (FN %.2f, UE %.2f): Setting up" : "Erbium (FN %.1f, UE %.2f): Setting up"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
    SetConsoleTitleA(GUI::windowTitle);


    //if constexpr (!FConfiguration::bGUI)
    //    Sleep(2000);

    printf("Hooking & finding offsets... (this may take a while)\n");

    FindNullsAndRetTrues();

    for (auto& NullFunc : NullFuncs)
        if (NullFunc != 0)
        {
            Utils::Patch<uint8_t>(NullFunc, 0xc3);
        }

    for (auto& RetTrueFunc : RetTrueFuncs)
    {
        if (RetTrueFunc == 0)
            continue;

        Utils::Patch<uint32_t>(RetTrueFunc, 0xc0ffc031);
        Utils::Patch<uint8_t>(RetTrueFunc + 4, 0xc3);
    }

    auto GameSessionPatch = FindGameSessionPatch();
    if (GameSessionPatch)
        Utils::Patch<uint8_t>(GameSessionPatch, 0x85);

    MH_Initialize();

    for (auto& HookFunc : _HookFuncs)
        HookFunc();

    MH_EnableHook(MH_ALL_HOOKS);

    *(bool*)FindGIsClient() = false;
    if (VersionInfo.EngineVersion > 4.20) // 3.6 and below have a crash on ALandscapeProxy
        *(bool*)FindGIsServer() = true;

    srand((uint32_t)time(0));

    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
    const wchar_t* terrainOpen = L"open Athena_Terrain";

    if (wcsstr(FConfiguration::Playlist, L"/MoleGame/Playlists/Playlist_MoleGame"))
    {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"Mole.WorstCasePlayerCount 1"), nullptr);
        terrainOpen = L"open Mole_UnderBase_Parent";
    }
    else if (VersionInfo.FortniteVersion >= 12.00 && wcsstr(FConfiguration::Playlist, L"/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2"))
        terrainOpen = L"open Creative_NoApollo_Terrain";
    else
    {
        if (VersionInfo.FortniteVersion >= 27.00)
        {
            if (VersionInfo.FortniteVersion >= 28.00)
                terrainOpen = L"open Helios_Terrain";
        }
        else if (VersionInfo.FortniteVersion >= 23.00)
            terrainOpen = L"open Asteria_Terrain";
        else if (VersionInfo.FortniteVersion >= 19.00)
            terrainOpen = L"open Artemis_Terrain";
        else if (VersionInfo.FortniteVersion >= 11.00)
            terrainOpen = L"open Apollo_Terrain";
    }

    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(terrainOpen), nullptr);

    auto EncryptionPatch = FindEncryptionPatch();
    if (EncryptionPatch)
        Utils::Patch<uint8_t>(EncryptionPatch, 0x74);
    else
        printf("Matchmaking is NOT supported on this version, please make a github issue.\n");

    for (auto& HookFunc : _PostLoadHookFuncs)
        HookFunc();

    MH_EnableHook(MH_ALL_HOOKS);

    Misc::bHookedAll = true;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        std::thread(Main).detach();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
