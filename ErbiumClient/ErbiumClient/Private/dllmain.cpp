// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "../Public/Client.h"
#include <thread>
#include "../../../Erbium/Erbium/Public/Configuration.h"

void ForceIris(uintptr_t IrisBool)
{
	Memcury::PE::Address add{ nullptr };

	const auto sizeOfImage = Memcury::PE::GetNTHeaders()->OptionalHeader.SizeOfImage;
	const auto scanBytes = reinterpret_cast<std::uint8_t*>(Memcury::PE::GetModuleBase());

	for (auto i = 0ul; i < sizeOfImage - 5; ++i)
	{
		if (scanBytes[i] == 0x83 || scanBytes[i] == 0x39)
		{
			if (Memcury::PE::Address(&scanBytes[i]).RelativeOffset(2, scanBytes[i] == 0x83).GetAs<void*>() == (void*)IrisBool)
			{
				add = Memcury::PE::Address(&scanBytes[i]);

                Utils::Patch<uint32_t>(__int64(&scanBytes[i]) + 2, 0x0); // the next bytes will always be greater than 0
			}
		}
	}
}

void Main()
{
    SDK::Init();

    if (VersionInfo.EngineVersion >= 5.0)
    {
        auto RuntimeOptions = DefaultObjImpl("FortRuntimeOptions");

        if (RuntimeOptions)
        {
            auto bWaitForServerToBeInitializedBeforeTravelingFeatureEnabledOffset = RuntimeOptions->GetOffset("bWaitForServerToBeInitializedBeforeTravelingFeatureEnabled");

            if (bWaitForServerToBeInitializedBeforeTravelingFeatureEnabledOffset != -1)
                *(bool*)(__int64(RuntimeOptions) + bWaitForServerToBeInitializedBeforeTravelingFeatureEnabledOffset) = false;
        }
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"log LogFortUIDirector None"), nullptr);
    }
    if (VersionInfo.EngineVersion >= 5.1)
    {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), FString(L"net.AllowEncryption 0"), nullptr);
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

    Client::Init();

    return;
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

