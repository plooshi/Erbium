// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "../Public/Utils.h"
#include <thread>
#include <iostream>
#include "../Public/Finders.h"
#include "../../FortniteGame/Public/FortInventory.h"


void Main()
{
    AllocConsole();
    FILE* s;
    freopen_s(&s, "CONOUT$", "w", stdout);

    printf("Initializing SDK...\n");
    SDK::Init();
    char buffer[67];
    sprintf_s(buffer, VersionInfo.EngineVersion >= 5.0 ? "Erbium (FN %.2f, UE %.1f): Setting up" : (VersionInfo.FortniteVersion >= 5.00 ? "Erbium (FN %.2f, UE %.2f): Setting up" : "Erbium (FN %.1f, UE %.2f): Setting up"), VersionInfo.FortniteVersion, VersionInfo.EngineVersion);
    SetConsoleTitleA(buffer);

    printf("Hooking & finding offsets...\n");
    FindNullsAndRetTrues();

    for (auto& NullFunc : NullFuncs)
        if (NullFunc != 0)
            Utils::Patch<uint8_t>(NullFunc, 0xc3);
    for (auto& RetTrueFunc : RetTrueFuncs) 
    {
        if (RetTrueFunc == 0)
            continue;

        Utils::Patch<uint32_t>(RetTrueFunc, 0xc0ffc031);
        Utils::Patch<uint8_t>(RetTrueFunc + 4, 0xc3);
    }

    MH_Initialize();

    for (auto& HookFunc : _HookFuncs) 
        HookFunc();

    MH_EnableHook(MH_ALL_HOOKS);

    *(bool*)FindGIsClient() = false;
    *(bool*)FindGIsServer() = true;

    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
    const wchar_t* terrainOpen = L"open Athena_Terrain";

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
    /*SDK::Init();

    auto Engine = UEngine::GetEngine();

    /*struct GameplayStatics_SpawnObject final
    {
    public:
        class UObject* ObjectClass;
        class UObject* Outer_0;
        class UObject* ReturnValue;
    };

    GameplayStatics_SpawnObject Parms{};
    Parms.ObjectClass = Engine->ConsoleClass;
    Parms.Outer_0 = Engine->GameViewport;

    UGameplayStatics::SpawnObject(&Parms);

    Engine->GameViewport->ViewportConsole = Parms.ReturnValue;* g/
    Engine->GameViewport->ViewportConsole = UGameplayStatics::SpawnObject(Engine->ConsoleClass, Engine->GameViewport);*/
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

