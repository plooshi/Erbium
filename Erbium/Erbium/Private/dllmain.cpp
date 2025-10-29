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
#include <FortniteGame/Public/FortPlayerControllerAthena.h>

void Main()
{
#ifndef CLIENT
    if constexpr (!FConfiguration::bGUI)
    {
        AllocConsole();
        FILE* s;
        freopen_s(&s, "CONOUT$", "w", stdout);
        freopen_s(&s, "CONOUT$", "w+", stderr);
        freopen_s(&s, "CONIN$", "r", stdin);
    }

    if constexpr (FConfiguration::bCustomCrashReporter)
        FCrashReporter::Register();

    printf("Initializing SDK...\n");
#endif
    SDK::Init();

#ifndef CLIENT
    if constexpr (FConfiguration::bGUI)
    {
        FILE* s;
        freopen_s(&s, "stdout.log", "w", stdout);
        freopen_s(&s, "stdout.log", "w+", stderr);

        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)GUI::Init, 0, 0, 0);
    }
#endif

    if (VersionInfo.EngineVersion >= 5.0)
    {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"log LogFortUIDirector None"), nullptr);
    }
    if (VersionInfo.EngineVersion >= 5.1)
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"net.AllowEncryption 0"), nullptr);
    if (VersionInfo.EngineVersion >= 5.3 && FConfiguration::bEnableIris)
    {
        auto IrisBool = Memcury::Scanner::FindPattern("83 3D ? ? ? ? ? 44 89 65").RelativeOffset(3).Get();
        *(uint32_t*)IrisBool = true;
        //UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"net.Iris.UseIrisReplication 1"), nullptr);
    }

#ifdef CLIENT
    Misc::InitClient();

    return;
#endif

    sprintf_s(GUI::windowTitle, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Setting up" : (VersionInfo.FortniteVersion >= 5.00 || VersionInfo.FortniteVersion < 1.2 ? "Erbium (FN %.2f, UE %.2f): Setting up" : "Erbium (FN %.1f, UE %.2f): Setting up"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
    SetConsoleTitleA(GUI::windowTitle);

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

    srand((uint32_t) time(0));

    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
    const wchar_t* terrainOpen = L"open Athena_Terrain";

    if (VersionInfo.FortniteVersion >= 11.00 && FConfiguration::bCreative)
        terrainOpen = L"open Creative_NoApollo_Terrain";
    else
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

BOOL APIENTRY DllMain( HMODULE hModule,
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

