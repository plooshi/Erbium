#pragma once
#include "pch.h"
#include "../../FortniteGame/Public/FortPlayerControllerAthena.h"
#include "../../FortniteGame/Public/BuildingSMActor.h"

uint64_t FindGIsClient()
{
    static uintptr_t GIsClient = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        std::vector<std::vector<uint8_t>> GIsClientLoads = {
            {0x88, 0x05},
            {0xC6, 0x05},
            {0x88, 0x1D},
            {0x44, 0x88},
            {0x40, 0x88}
        };

        auto sRef = Memcury::Scanner::FindStringRef(L"AllowCommandletRendering").Get();
        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"llowCommandletRendering").Get(); // bro why

        int Skip = 2;
        uint8_t correctByte = 0;
        for (int i = 0; i < 0x100; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            for (auto& Load : GIsClientLoads)
            {
                bool bMatches = true;
                for (int x = 0; x < Load.size(); x++)
                {
                    if (Load[x] != *(Ptr + x))
                    {
                        bMatches = false;
                        break;
                    }
                }
                if (!bMatches) continue;
                if ((Load[0] == 0x44 || Load[0] == 0x40) && (*(Ptr + 2) == 0x74 || *(Ptr + 2) == 0x30)) continue;
                if (!correctByte)
                    correctByte = Load[0];
                else if (Load[0] != correctByte)
                    continue;
                if (Skip > 0)
                {
                    Skip--;
                    continue;
                }

                return GIsClient = Memcury::Scanner(Ptr).RelativeOffset((Load[0] == 0x44 || Load[0] == 0x40) ? 3 : 2, Load[0] == 0xc6).Get();
            }
        }
    }

    return GIsClient;
}

uint64_t FindGIsServer()
{
    static uintptr_t GIsServer = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        std::vector<std::vector<uint8_t>> GIsServerLoads = {
            {0x88, 0x05},
            {0xC6, 0x05},
            {0x88, 0x1D},
            {0x44, 0x88},
            {0x40, 0x88}
        };

        auto sRef = Memcury::Scanner::FindStringRef(L"AllowCommandletRendering").Get();
        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"llowCommandletRendering").Get(); // bro why

        int Skip = 1;
        uint8_t correctByte = 0;
        for (int i = 0; i < 0x100; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            for (auto& Load : GIsServerLoads)
            {
                bool bMatches = true;
                for (int x = 0; x < Load.size(); x++)
                {
                    if (Load[x] != *(Ptr + x))
                    {
                        bMatches = false;
                        break;
                    }
                }
                if (!bMatches) continue;
                if ((Load[0] == 0x44 || Load[0] == 0x40) && (*(Ptr + 2) == 0x74 || *(Ptr + 2) == 0x30)) continue;
                if (!correctByte)
                    correctByte = Load[0];
                else if (Load[0] != correctByte)
                    continue;
                if (Skip > 0)
                {
                    Skip--;
                    continue;
                }

                return GIsServer = Memcury::Scanner(Ptr).RelativeOffset((Load[0] == 0x44 || Load[0] == 0x40) ? 3 : 2, Load[0] == 0xc6).Get();
            }
        }
    }

    return GIsServer;
}

uint64_t FindGetNetMode()
{
    static uint64_t GetNetMode = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (floor(VersionInfo.FortniteVersion) == 18)
            GetNetMode = Memcury::Scanner::FindPattern("48 83 EC 28 48 83 79 ? ? 75 20 48 8B 91 ? ? ? ? 48 85 D2 74 1E 48 8B 02 48 8B CA FF 90").Get();
        else {
            auto sRef = Memcury::Scanner::FindStringRef(L"PREPHYSBONES").Get();

            for (int i = 0; i < 1000; i++)
            {
                auto Ptr = (uint8_t*)(sRef - i);

                if (*Ptr == 0x40 && *(Ptr + 1) == 0x55)
                {
                    GetNetMode = uint64_t(Ptr);
                    break;
                }
            }

            for (int i = 0; i < 400; i++)
            {
                auto Ptr = (uint8_t*)(GetNetMode + i);

                if (*Ptr == 0xe8 && *(Ptr - 1) != 0x8b && *(Ptr - 1) != 0xe7 && *(Ptr - 1) != 0x24)
                {
                    GetNetMode = uint64_t(Ptr);
                    break;
                }
            }

            GetNetMode = Memcury::Scanner(GetNetMode).RelativeOffset(1).Get();
        }
    }

    return GetNetMode;
}

uint64_t FindGetWorldContext()
{
    static uint64_t GetWorldContext = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        GetWorldContext = Memcury::Scanner::FindPattern("48 8B 81 ? ? ? ? 48 63 89 ? ? ? ? 4C 8D 04 C8 49 3B C0 74 ? 48 8B 08 48 39 91 80 02 00 00 75 ? 48 8B C1 C3").Get();

        if (!GetWorldContext)
            GetWorldContext = Memcury::Scanner::FindPattern("48 8B 81 ? ? ? ? 48 63 89 ? ? ? ? 4C 8D 04 C8 49 3B C0 74 ? 48 8B 08 48 39 91 B8 02 00 00 75 ? 48 8B C1 C3").Get();

        if (!GetWorldContext)
            GetWorldContext = Memcury::Scanner::FindPattern("48 8B 81 ? ? ? ? 48 63 89 ? ? ? ? 4C 8D 04 C8 49 3B C0 74 ? 48 8B 08 48 39 91 70 02 00 00 75 ? 48 8B C1 C3").Get();

        if (!GetWorldContext)
            GetWorldContext = Memcury::Scanner::FindPattern("48 8B 81 ? ? ? ? 48 63 89 ? ? ? ? 4C 8D 04 C8 49 3B C0 74 ? 48 8B 08 48 39 91 ? ? ? ? 75 ? 48 8B C1 C3").Get();

        if (!GetWorldContext)
            GetWorldContext = Memcury::Scanner::FindPattern("48 8B 81 ? ? ? ? 48 63 89 ? ? ? ? 4C 8D 04 C8 49 3B C0 74 ? 48 8B 08 48 39 91 B8 02 00 00 74 ? 48 83 C0 08 EB ??").Get();

        if (!GetWorldContext)
            GetWorldContext = Memcury::Scanner::FindPattern("48 8B 81 ? ? ? ? 48 63 89 ? ? ? ? 4C 8D 04 C8 49 3B C0 74 ? 48 8B 08 48 39 91 ? ? ? ? 74 ? 48 83 C0 08 EB ??").Get();

        if (!GetWorldContext)
            GetWorldContext = Memcury::Scanner::FindPattern("40 53 48 83 EC ?? F6 41 08 10 48 8B D9 75 ?? 48 8B 41 20").Get();
    }
    return GetWorldContext;
}

uint64_t FindCreateNetDriver()
{
    static uint64_t CreateNetDriver = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        CreateNetDriver = Memcury::Scanner::FindPattern("49 8B D8 48 8B F9 E8 ?? ?? ?? ?? 48 8B D0 4C 8B C3 48 8B CF 48 8B 5C 24 ?? 48 83 C4 ?? 5F E9 ?? ?? ?? ??").Get();
        if (!CreateNetDriver)
        {
            CreateNetDriver = Memcury::Scanner::FindPattern("E8 ?? ?? ?? ?? 4C 8B 44 24 ?? 48 8B D0 48 8B CB E8 ?? ?? ?? ?? 48 83 C4 ?? 5B C3").Get();
            if (!CreateNetDriver)
                CreateNetDriver = Memcury::Scanner::FindPattern("33 D2 E8 ?? ?? ?? ?? 48 8B D0 4C 8B C3 48 8B CF E8 ?? ?? ?? ?? 48 8B 5C 24 ?? 48 83 C4 ?? 5F C3").Get();
        }

        if (CreateNetDriver)
        {
            for (int i = 0; i < 0x200; i++)
            {
                auto Ptr = (uint8_t*)(CreateNetDriver - i);

                if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c)
                {
                    CreateNetDriver = uint64_t(Ptr);
                    break;
                }
                else if (*Ptr == 0x4C && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x44)
                {
                    CreateNetDriver = uint64_t(Ptr);
                    break;
                }
            }
        }
    }

    return CreateNetDriver;
}

uint64_t FindCreateNetDriverWorldContext()
{
    static uint64_t CreateNetDriver = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (std::floor(VersionInfo.FortniteVersion) == 19)
            return CreateNetDriver = Memcury::Scanner::FindPattern("41 56 48 83 EC ? 48 63 81 ? ? ? ? 48 8D ? ? ? ? ? 48 8B B9 ? ? ? ? 4C 8B F2").ScanFor({ 0xC3 }, false).ScanFor({ 0x48 }).Get();
        if (VersionInfo.FortniteVersion >= 20)
        {
            auto CreateNetDriver_ = Memcury::Scanner::FindPattern("41 56 41 57 48 83 EC ? 48 63 81 ? ? ? ? 48 8D ? ? ? ? ? 48 8B B9");

            if (!CreateNetDriver_.Get())
                CreateNetDriver_ = Memcury::Scanner::FindPattern("41 56 41 57 48 8B EC 48 83 EC ? 48 63 81 ? ? ? ? 48 8D ? ? ? ? ? 48 8B B1");

            if (!CreateNetDriver_.Get())
                CreateNetDriver_ = Memcury::Scanner::FindPattern("41 56 41 57 48 8B EC 48 83 EC ? 48 63 81 ? ? ? ? 48 8D ? ? ? ? ? 4C 8B B1");

            if (!CreateNetDriver_.Get())
                CreateNetDriver_ = Memcury::Scanner::FindPattern("41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 63 81 ? ? ? ? 4C 8D 3D");

            if (!CreateNetDriver_.Get())
                CreateNetDriver_ = Memcury::Scanner::FindPattern("41 56 41 57 48 83 EC ? 4C 8B EA 4C 8B E1 48 81 C1");

            auto StartOfFuncBefore = CreateNetDriver_.Get();
            if (CreateNetDriver_.Get())
            {
                for (int i = 0; i < 2048; i++)
                {
                    auto Ptr = (uint8_t*)(StartOfFuncBefore - i);

                    if (*Ptr == 0xC3)
                    {
                        StartOfFuncBefore = uint64_t(Ptr);
                        break;
                    }
                    else if (VersionInfo.FortniteVersion >= 25 && *Ptr == 0xE9)
                    {
                        StartOfFuncBefore = uint64_t(Ptr);
                        break;
                    }
                }

                CreateNetDriver = Memcury::Scanner(StartOfFuncBefore).ScanFor({0x48}).Get();
            }

            return CreateNetDriver;
        }

        auto Ptr = Memcury::Scanner::FindPattern("C7 44 24 ? 00 20 00 00 33 D2 48 8B C8 E8 ? ? ? ? 48 ?? 4C");

        if (!Ptr.Get())
            Ptr = Memcury::Scanner::FindPattern("C7 44 24 ? 00 20 00 00 48 8B C8 E8 ? ? ? ? 48 ?? 4C");

        if (!Ptr.Get())
            return 0;

        auto scuffness = Ptr.ScanFor({ 0x48, 0x83, 0xEC }, false).ScanFor({ 0xC3 }, false).ScanFor({ 0x48 }); // should land us right at the start of CreateNetDriver

        return CreateNetDriver = scuffness.Get();
    }

    return CreateNetDriver;
}

uint64_t FindInitListen()
{
    static uint64_t InitListen = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion >= 5.0)
        {
            InitListen = Memcury::Scanner::FindPattern("4D 8B C8 4C 8B C2 33 D2 FF 90 ? ? ? ? 84 C0 75 ? 80 3D").ScanFor({ 0x4C, 0x8B, 0xDC }, false).Get();
            if (!InitListen)
            {
                InitListen = Memcury::Scanner::FindPattern("4C 8B DC 49 89 5B ? 49 89 73 ? 57 48 83 EC ? 48 8B BC 24").Get();

                if (!InitListen)
                    InitListen = Memcury::Scanner::FindPattern("4C 8B DC 49 89 5B 08 49 89 73 10 57 48 83 EC 40 48 8B 7C 24 ? 49 8B F0 48 8B 01 48 8B D9 49 89 7B E0 45").Get();
            }
        }
        else if (VersionInfo.EngineVersion >= 4.27)
            InitListen = Memcury::Scanner::FindPattern("4C 8B DC 49 89 5B 08 49 89 73 10 57 48 83 EC 50 48 8B BC 24 ? ? ? ? 49 8B F0 48 8B 01 48 8B").Get();
        else if (VersionInfo.FortniteVersion == 1.91)
            InitListen = ImageBase + 0x3A5D200;
        else {
            auto sRef = Memcury::Scanner::FindStringRef(L"%s IpNetDriver listening on port %i").Get();
            int skip = 1;
            for (int i = 0; i < 2000; i++)
            {
                auto Ptr = (uint8_t*)(sRef - i);

                if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c)
                {
                    if (skip > 0)
                    {
                        skip--;
                        continue;
                    }
                    InitListen = uint64_t(Ptr);
                    break;
                }
            }
        }
    }

    return InitListen;
}

uint64_t FindSetWorld()
{
    static uint64_t SetWorld = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        SetWorld = VersionInfo.FortniteVersion <= 13.20 ? Memcury::Scanner::FindStringRef(L"AOnlineBeaconHost::InitHost failed").ScanFor({ 0x48, 0x8B, 0xD0, 0xE8 }, false).RelativeOffset(4).Get() : 0;

        if (VersionInfo.FortniteVersion >= 25)
        {
            SetWorld = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 4C 89 70 ? 41 57 48 83 EC ? 48 8B FA 4C 8D B1 ? ? ? ? 48 8B 91").Get();

            if (!SetWorld)
                SetWorld = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 4C 8D B9 ? ? ? ? 48 8B FA").Get();
        }
        else if (VersionInfo.FortniteVersion >= 19)
            SetWorld = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC ? 48 8B FA 48 8B D9 48 8B 91 ? ? ? ? 48 85 D2 75").Get();
        else if (VersionInfo.FortniteVersion > 13.20)
        {
            auto Season = (int)floor(VersionInfo.FortniteVersion);
            uint32 VftIdx = 0;
            switch (Season)
            {
            case 13:
                VftIdx = 0x70;
                break;
            case 18:
                VftIdx = 0x73;
                break;
            case 19:
                VftIdx = 0x7a;
                break;
            case 22:
            case 23:
            case 20:
                VftIdx = 0x7b;
                break;
            case 21:
                VftIdx = 0x7c;
                break;
            case 24:
                VftIdx = 0x7d;
                break;
            default:
                if (VersionInfo.FortniteVersion >= 14 && VersionInfo.FortniteVersion <= 15.2)
                    VftIdx = 0x71;
                else if (VersionInfo.FortniteVersion >= 15.3 && VersionInfo.FortniteVersion < 18)
                    VftIdx = 0x72;

                break;
            }
            SetWorld = uintptr_t(DefaultObjImpl("NetDriver")->Vft[VftIdx]);
        }
    }

    return SetWorld;
}

uint64_t FindTickFlush()
{
    static uint64_t TickFlush = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        TickFlush = VersionInfo.EngineVersion == 4.16 ? Memcury::Scanner::FindPattern("4C 8B DC 55 53 56 57 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 41 0F 29 7B").Get() : 0;

        if (VersionInfo.EngineVersion == 4.19)
            TickFlush = Memcury::Scanner::FindPattern("4C 8B DC 55 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 45 0F 29 43 ? 45 0F 29 4B ? 48 8B 05 ? ? ? ? 48").Get();

        else if (VersionInfo.EngineVersion >= 4.27 && VersionInfo.EngineVersion < 5.0)
        {
            TickFlush = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 B8 0F 29 78 A8 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 8A").Get();

            if (!TickFlush)
                TickFlush = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 B8 0F 29 78 A8 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 44 0F").Get();

            if (!TickFlush)
                TickFlush = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 B8 0F 29 78 A8 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B F9 48 89 4D 38 48 8D 4D 40").Get();
        }
        else if (!TickFlush)
        {
            auto sRef = Memcury::Scanner::FindStringRef(L"STAT_NetTickFlush", false).Get();
            if (!sRef && VersionInfo.EngineVersion == 4.20)
                TickFlush = Memcury::Scanner::FindPattern("4C 8B DC 55 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 45 0F 29 43 ? 45 0F 29 4B ? 48 8B 05 ? ? ? ? 48").Get();
            else
                for (int i = 0; i < 1000; i++)
                {
                    auto Ptr = (uint8_t*)(sRef - i);

                    if (*Ptr == 0x48 && *(Ptr + 1) == 0x8b && *(Ptr + 2) == 0xc4)
                    {
                        TickFlush = uint64_t(Ptr);
                        break;
                    }
                    else if (*Ptr == 0x4c && *(Ptr + 1) == 0x8b && *(Ptr + 2) == 0xdc)
                    {
                        TickFlush = uint64_t(Ptr);
                        break;
                    }
                    else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c)
                    {
                        TickFlush = uint64_t(Ptr);
                        break;
                    }
                    else if (*Ptr == 0x40 && *(Ptr + 1) == 0x55)
                    {
                        TickFlush = uint64_t(Ptr);
                        break;
                    }
                }
        }
    }

    return TickFlush;
}

int32_t FindIsNetRelevantForVft()
{
    static int32_t IsNetRelevantForIdx = -1;

    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"Actor %s / %s has no root component in AActor::IsNetRelevantFor. (Make bAlwaysRelevant=true?)", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        uintptr_t IsNetRelevantFor = 0;
        for (int i = 0; i < 2048; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            if (*Ptr == 0x48 && *(Ptr + 1) == 0x8b && *(Ptr + 2) == 0xc4)
            {
                IsNetRelevantFor = uint64_t(Ptr);
                break;
            }
            else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c)
            {
                IsNetRelevantFor = uint64_t(Ptr);
                break;
            }
            else if (*Ptr == 0x40 && *(Ptr + 1) == 0x55)
            {
                IsNetRelevantFor = uint64_t(Ptr);
                break;
            }
        }
        
        if (!IsNetRelevantFor)
            return 0;

        auto ActorVft = AActor::GetDefaultObj()->Vft;

        for (int i = 0; i < 500; i++)
        {
            if (ActorVft[i] == (void*)IsNetRelevantFor)
            {
                return IsNetRelevantForIdx = i;
            }
        }
        return IsNetRelevantForIdx = 0;
    }

    return IsNetRelevantForIdx;
}

uint64_t FindServerReplicateActors()
{
    static uint64_t ServerReplicateActors = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        int ServerReplicateActorsVft = 0;
        switch ((int)floor(VersionInfo.FortniteVersion))
        {
        case 3:
            if (VersionInfo.FortniteVersion >= 3.4)
                ServerReplicateActorsVft = 0x53;
            break;
        case 4:
            ServerReplicateActorsVft = 0x53;
            break;
        case 5:
            ServerReplicateActorsVft = 0x54;
            break;
        case 6:
            ServerReplicateActorsVft = 0x56;
            break;
        case 10:
        case 9:
        case 8:
        case 7:
            if (VersionInfo.FortniteVersion >= 7.40 && VersionInfo.FortniteVersion <= 8.40 && VersionInfo.FortniteVersion != 8.30)
                ServerReplicateActorsVft = 0x57;
            else
                ServerReplicateActorsVft = 0x56;
            break;
        case 11:
            if (VersionInfo.FortniteVersion >= 11 && VersionInfo.FortniteVersion <= 11.10)
                ServerReplicateActorsVft = 0x57;
            else if (VersionInfo.FortniteVersion == 11.30 || VersionInfo.FortniteVersion == 11.31)
                ServerReplicateActorsVft = 0x59;
            else
                ServerReplicateActorsVft = 0x5a;
            break;
        case 12:
        case 13:
            ServerReplicateActorsVft = 0x5d;
            break;
        case 18:
        case 17:
        case 16:
        case 15:
        case 14:
            if (VersionInfo.FortniteVersion <= 15.2)
                ServerReplicateActorsVft = 0x5e;
            else
                ServerReplicateActorsVft = 0x5f;
            break;
        case 19:
            if (VersionInfo.FortniteVersion >= 19.2)
                ServerReplicateActorsVft = 0x65;
            else
                ServerReplicateActorsVft = 0x66;
            break;
        }
        if (ServerReplicateActorsVft)
        {
            ServerReplicateActors = uint64_t(DefaultObjImpl("FortReplicationGraph")->Vft[ServerReplicateActorsVft]);
        }
    }
    return ServerReplicateActors;
}

uint64_t FindSendRequestNow()
{
    static uint64_t SendRequestNow = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"MCP-Profile: Dispatching request to %s", true, 0, VersionInfo.FortniteVersion >= 19).Get();
        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"MCP-Profile: Dispatching request to %s - ContextCredentials: %s", true, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 1000; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c)
            {
                SendRequestNow = uint64_t(Ptr);
                break;
            }
            else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8b && *(Ptr + 2) == 0xc4)
            {
                SendRequestNow = uint64_t(Ptr);
                break;
            }
        }
    }

    return SendRequestNow;
}

uint64 FindGetMaxTickRate()
{
    static uint64_t GetMaxTickRate = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion >= 5.0)
        {
            GetMaxTickRate = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 57 41 55 41 56 48 83 EC ? 0F 29 70 ? 48 8B D9 0F 29").Get();

            if (!GetMaxTickRate)
                GetMaxTickRate = Memcury::Scanner::FindPattern("40 53 48 83 EC 50 0F 29 74 24 ? 48 8B D9 0F 29 7C 24 ? 0F 28 F9 44 0F 29").Get();

            if (!GetMaxTickRate)
                GetMaxTickRate = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 57 41 54 41 56 48 83 EC ? 0F 29 70 ? 48 8B D9").Get();

            return GetMaxTickRate;
        }

        if (VersionInfo.EngineVersion >= 4.27)
            return GetMaxTickRate = Memcury::Scanner::FindPattern("40 53 48 83 EC 60 0F 29 74 24 ? 48 8B D9 0F 29 7C 24 ? 0F 28").Get();

        auto sRef = Memcury::Scanner::FindStringRef(L"Hitching by request!").Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 400; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x53)
            {
                GetMaxTickRate = sRef - i;
                break;
            }
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
            {
                GetMaxTickRate = sRef - i;
                break;
            }
        }
    }

    return GetMaxTickRate;
}

uint64_t FindGiveAbility()
{
    static uint64_t GiveAbility = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion <= 4.20)
            return GiveAbility = Memcury::Scanner::FindPattern("48 89 5C 24 ? 56 57 41 56 48 83 EC 20 83 B9").Get();
        else if (VersionInfo.EngineVersion == 4.21)
            return GiveAbility = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 7C 24 ? 41 56 48 83 EC 20 83 B9 ? ? ? ? ? 49 8B E8 4C 8B F2").Get();
        else if (VersionInfo.EngineVersion >= 5.3)
            return GiveAbility = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 49 8B 40 ? 45 33 E4").Get();
        else if (VersionInfo.EngineVersion >= 5.0)
        {
            GiveAbility = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC 20 8B 81 ? ? ? ? 49 8B E8 4C").Get();

            if (!GiveAbility)
                GiveAbility = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 56 41 57 48 8B EC 48 83 EC ? 49 8B 40").Get();

            if (!GiveAbility)
                GiveAbility = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC 30 49 8B 40 10 45 33 F6 49 8B E8 48 8B F2 48 8B").Get();

            return GiveAbility;
        }

        Memcury::Scanner addr = Memcury::Scanner::FindStringRef(L"GiveAbilityAndActivateOnce called on ability %s on the client, not allowed!", true, 1, VersionInfo.EngineVersion >= 5.0);

        GiveAbility = addr.ScanFor({ 0xE8 }).RelativeOffset(1).Get();
    }

    return GiveAbility;
}

uint64_t FindConstructAbilitySpec()
{
    static uint64_t ConstructAbilitySpec = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion >= 4.20 && VersionInfo.EngineVersion <= 4.24)
            return ConstructAbilitySpec = Memcury::Scanner::FindPattern("80 61 29 F8 48 8B 44 24 ?").Get();
        else if (VersionInfo.EngineVersion == 4.25)
        {
            ConstructAbilitySpec = Memcury::Scanner::FindPattern("48 8B 44 24 ? 80 61 29 F8 80 61 31 FE 48 89 41 20 33 C0 89 41", false).Get();

            if (!ConstructAbilitySpec)
                ConstructAbilitySpec = Memcury::Scanner::FindPattern("80 61 29 F8 48 8B 44 24 ?").Get();
        }
        else if (VersionInfo.EngineVersion == 4.26)
            return ConstructAbilitySpec = Memcury::Scanner::FindPattern("80 61 31 FE 0F 57 C0 80 61 29 F0 48 8B 44 24 ? 48").Get();
        else if (VersionInfo.EngineVersion == 4.27)
            return ConstructAbilitySpec = Memcury::Scanner::FindPattern("80 61 31 FE 41 83 C9 FF 80 61 29 F0 48 8B 44 24 ? 48 89 41").Get();
        else if (VersionInfo.EngineVersion == 5.0)
            return ConstructAbilitySpec = Memcury::Scanner::FindPattern("4C 8B C9 48 8B 44 24 ? 83 C9 FF 41 80 61 ? ? 41 80 61 ? ? 49 89 41 20 33 C0 41 88 41 30 49 89 41").Get();
        else if (VersionInfo.EngineVersion >= 5.4)
        {
            ConstructAbilitySpec = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 83 EC ? 41 83 CE ? 33 ED 40 38 2D ? ? ? ? 41 8B F8").Get();

            if (!ConstructAbilitySpec)
                ConstructAbilitySpec = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 83 EC ? 41 83 CE").Get();
        }
        else if (VersionInfo.EngineVersion >= 5.1)
        {
            ConstructAbilitySpec = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 83 EC ? 48 8B 44 24 ? 41 83 CE").Get();

            if (!ConstructAbilitySpec)
                ConstructAbilitySpec = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 44 89 48 ? 57 48 83 EC ? 48 8B 44 24").Get();
        }
    }

    return ConstructAbilitySpec;
}


uint64_t FindInternalTryActivateAbility()
{
    static uint64_t InternalTryActivateAbility = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"InternalTryActivateAbility called with invalid Handle! ASC: %s. AvatarActor: %s", true, 0, VersionInfo.FortniteVersion >= 16).Get();

        for (int i = 0; i < 1000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                return InternalTryActivateAbility = sRef - i;

            else if (*(uint8_t*)(sRef - i) == 0x4C && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x4C)
                return InternalTryActivateAbility = sRef - i;
            
            else if (VersionInfo.FortniteVersion >= 25 && *(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                return InternalTryActivateAbility = sRef - i;
        }
    }

    return InternalTryActivateAbility;
}


uint64 FindApplyCharacterCustomization()
{
    static uint64_t ApplyCharacterCustomization = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.FortniteVersion == 17.50)
            return ApplyCharacterCustomization = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 80 B9 ? ? ? ? ? 4C 8B EA").Get();
        else if (VersionInfo.FortniteVersion >= 24)
        {
            ApplyCharacterCustomization = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 80 B9 ? ? ? ? ? 48 8B F1").Get();

            if (!ApplyCharacterCustomization)
                ApplyCharacterCustomization = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 80 B9 ? ? ? ? ? 48 8B C2").Get();

            if (!ApplyCharacterCustomization)
                ApplyCharacterCustomization = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 81 EC ? ? ? ? 80 B9").Get();

            return ApplyCharacterCustomization;
        }
        else if (std::floor(VersionInfo.FortniteVersion) == 22)
            return ApplyCharacterCustomization = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 81 EC ? ? ? ? 80 B9").Get();

        auto sRef = Memcury::Scanner::FindStringRef(L"AFortPlayerState::ApplyCharacterCustomization - Failed initialization, using default parts. Player Controller: %s PlayerState: %s, HeroId: %s", false, 0, VersionInfo.FortniteVersion >= 17, VersionInfo.FortniteVersion < 20.40 && VersionInfo.FortniteVersion != 19.01).Get();

        if (!sRef)
            return 0;

        for (int i = 100; i < 7000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x40 && (*(uint8_t*)(sRef - i + 1) == 0x53 || *(uint8_t*)(sRef - i + 1) == 0x55))
                return ApplyCharacterCustomization = sRef - i;

            if (VersionInfo.FortniteVersion >= 15)
            {
                if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                    return ApplyCharacterCustomization = sRef - i;
            }

            if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                return ApplyCharacterCustomization = sRef - i;

            if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x54 && *(uint8_t*)(sRef - i + 5) == 0x55) // 10.40
                return ApplyCharacterCustomization = sRef - i;
        }

        ApplyCharacterCustomization = Memcury::Scanner::FindPattern("48 8B C4 48 89 50 10 55 57 48 8D 68 A1 48 81 EC ? ? ? ? 80 B9").Get();
    }

    return ApplyCharacterCustomization;
}

uint64 FindHandlePostSafeZonePhaseChanged()
{
    static uint64_t HandlePostSafeZonePhaseChanged = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion == 4.16)
            return HandlePostSafeZonePhaseChanged = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B B9 ? ? ? ? 33 DB 0F 29 74 24").Get();
        else if (VersionInfo.EngineVersion == 4.19)
            return HandlePostSafeZonePhaseChanged = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 70 48 8B B9 ? ? ? ? 33 DB 0F 29 74 24 ? 48 8B F1 48 85 FF 74 2C E8").Get();
        else if (VersionInfo.EngineVersion == 4.20)
            return HandlePostSafeZonePhaseChanged = Memcury::Scanner::FindPattern("E8 ? ? ? ? EB 31 80 B9 ? ? ? ? ?").RelativeOffset(1).Get(); // 3.5
        else if (VersionInfo.FortniteVersion >= 7 && VersionInfo.FortniteVersion <= 8)
            return HandlePostSafeZonePhaseChanged = Memcury::Scanner::FindPattern("E9 ? ? ? ? 48 8B C1 40 38 B9").RelativeOffset(1).Get();
        else if (VersionInfo.EngineVersion == 4.23)
            return HandlePostSafeZonePhaseChanged = Memcury::Scanner::FindPattern("E8 ? ? ? ? EB 42 80 BA").RelativeOffset(1).Get();

        auto sRef = Memcury::Scanner::FindStringRef(L"FortGameModeAthena: No MegaStorm on SafeZone[%d].  GridCellThickness is less than 1.0.", true, 0, VersionInfo.EngineVersion >= 4.27).Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 15000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x40 && (*(uint8_t*)(sRef - i + 1) == 0x53 || *(uint8_t*)(sRef - i + 1) == 0x55))
                return HandlePostSafeZonePhaseChanged = sRef - i;
            else if ((VersionInfo.FortniteVersion < 8 || std::floor(VersionInfo.FortniteVersion) == 11) && *(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                return HandlePostSafeZonePhaseChanged = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                return HandlePostSafeZonePhaseChanged = sRef - i;
        }

        return 0;
    }

    return HandlePostSafeZonePhaseChanged;
}

uint64 FindSpawnLoot()
{
    static uint64 SpawnLoot = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"ABuildingContainer::SpawnLoot() called on %s (%s)...", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"ABuildingContainer::SpawnLoot() called on %s (%s)", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"ABuildingContainer::SpawnLoot() >>> START called on %s (%s)", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 0x1000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x40 && (*(uint8_t*)(sRef - i + 1) == 0x53 || *(uint8_t*)(sRef - i + 1) == 0x55))
                return SpawnLoot = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                return SpawnLoot = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                return SpawnLoot = sRef - i;
        }
    }

    return SpawnLoot;
}

uint64_t FindFinishedTargetSpline()
{
    static uint64 FinishedTargetSpline = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion == 4.16 || VersionInfo.EngineVersion == 4.19)
            return Memcury::Scanner::FindPattern("4C 8B DC 53 55 56 48 83 EC 60 48 8B F1 48 8B 89 ? ? ? ? 48 85 C9").Get();
        else if (VersionInfo.EngineVersion == 4.20)
        {
            FinishedTargetSpline = Memcury::Scanner::FindPattern("4C 8B DC 53 55 56 48 83 EC 60 48 8B F1 48 8B 89", false).Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 20 48 8B D9 48 8B 89 ? ? ? ? 48 85 C9 74 20 48 8D 44 24").Get();

            return FinishedTargetSpline;
        }
        else if (VersionInfo.EngineVersion == 4.21)
        {
            FinishedTargetSpline = Memcury::Scanner::FindPattern("40 53 56 48 83 EC 38 4C 89 6C 24 ? 48 8B F1 4C 8B A9", false).Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("40 53 56 57 48 83 EC 30 4C 89 6C 24 ? 48 8B F1 4C 8B A9 ? ? ? ? 4D 85 ED 0F 84").Get();

            return FinishedTargetSpline;
        }
        else if (VersionInfo.EngineVersion == 4.22)
            return FinishedTargetSpline = Memcury::Scanner::FindPattern("40 53 56 57 48 83 EC 30 4C 89 6C 24 ? 48 8B F1 4C 8B A9 ? ? ? ? 4D 85 ED 0F 84").Get();
        else if (VersionInfo.EngineVersion >= 4.23 && VersionInfo.EngineVersion <= 4.26)
            return FinishedTargetSpline = Memcury::Scanner::FindPattern("40 53 56 48 83 EC 38 4C 89 6C 24 ? 48 8B F1 4C 8B A9 ? ? ? ? 4D 85 ED").Get();
        else if (VersionInfo.EngineVersion == 4.27)
        {
            FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 54 41 56 41 57 48 83 EC 20 48 8B B1 ? ? ? ? 48 8B D9 48 85 F6", false).Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B B9 ? ? ? ? 48 8B D9 48 85 FF 74 16 48 89", false).Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 10 48 89 68 18 57 48 83 EC 20 48 8B D9 48 8B 89 ? ? ? ? 48 85").Get();

            return FinishedTargetSpline;
        }
        else if (VersionInfo.EngineVersion == 5.0)
        {
            FinishedTargetSpline = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B B9", false).Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 48 8D AC 24 ? ? ? ? 48 81 EC A0 01 00 00").Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B B9 ? ? ? ? 45 33 E4 48 8B D9 48 85 FF 74 0F").Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B B9 ? ? ? ? 48 8B D9 48 85 FF 0F 84").Get();

            return FinishedTargetSpline;
        }
        else if (VersionInfo.EngineVersion == 5.1)
        {
            FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 55 41 56 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 44 8B 91").Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 44 8B 81 ? ? ? ? 48 8B F9").Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 89 5C ?? ?? 48 89 74 ?? ?? 55 57 41 55 41 56 41 57 48 8D AC ?? ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 ?? ?? ?? ?? 44 8B 91 ?? ?? ?? ??").Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 44 8B 81").Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 44 8B 89").Get();
        }
        else if (VersionInfo.EngineVersion == 5.2)
            FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 44 8B 81 ? ? ? ? 48 8B D9 BE").Get();
        else if (VersionInfo.EngineVersion >= 5.3)
        {
            FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 8B E1 48 89 4C 24 ? 48 81 C1").Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 8B F9 48 89 4C 24 ? 48 81 C1").Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B F9 48 89 4C 24 ? 48 81 C1 ? ? ? ? E8").Get();

            if (!FinishedTargetSpline)
                FinishedTargetSpline = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 8B F1 48 89 4C 24 ? 48 81 C1 ? ? ? ? E8").Get();
        }
    }

    return FinishedTargetSpline;
}


uint64_t FindPickTeam()
{
    static uint64_t PickTeam = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion == 4.26)
        {
            PickTeam = Memcury::Scanner::FindPattern("88 54 24 10 53 56 41 54 41 55 41 56 48 83 EC 60 4C 8B A1", false).Get();

            if (!PickTeam)
                PickTeam = Memcury::Scanner::FindPattern("88 54 24 10 53 55 56 41 55 41 ? 48 83 EC 70 48", false).Get();

            if (PickTeam)
                return PickTeam;
        }
        else if (VersionInfo.FortniteVersion == 7.20 || VersionInfo.FortniteVersion == 7.30)
            return PickTeam = Memcury::Scanner::FindPattern("89 54 24 10 53 56 41 54 41 55 41 56 48 81 EC").Get();

        auto Addr = Memcury::Scanner::FindStringRef(L"PickTeam for [%s] used beacon value [%d]", true, 0, VersionInfo.FortniteVersion >= 19);

        if (!Addr.Get())
            Addr = Memcury::Scanner::FindStringRef(L"PickTeam for [%s] used beacon value [%s]", true, 0, VersionInfo.FortniteVersion >= 19);

        PickTeam = Addr.ScanFor(VersionInfo.FortniteVersion <= 4.1 ? std::vector<uint8_t>{ 0x48, 0x89, 0x6C } : (VersionInfo.FortniteVersion >= 16 ? std::vector<uint8_t>{ 0x48, 0x89, 0x5C } : std::vector<uint8_t>{ 0x40, 0x55 }), false).Get();
    }

    return PickTeam;
}

uint64_t FindCantBuild()
{
    static uint64_t CantBuild = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        CantBuild = Memcury::Scanner::FindPattern("48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 41 56 48 83 EC ? 49 8B E9 4D 8B F0", false).Get();

        if (!CantBuild)
            CantBuild = Memcury::Scanner::FindPattern("48 89 54 24 ? 55 56 41 56 48 83 EC 50", false).Get();

        if (!CantBuild)
            CantBuild = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 60 4D 8B F1 4D 8B F8", false).Get();

        if (!CantBuild)
            CantBuild = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 60 49 8B E9 4D 8B F8 48 8B DA 48 8B F9 BE ? ? ? ? 48", false).Get();

        if (!CantBuild)
            CantBuild = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 70 49 8B E9 4D 8B F8 48 8B DA 48 8B F9").Get();

        if (!CantBuild)
            CantBuild = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B 1A 4D 8B F1").Get();
    }

    return CantBuild;
}

uint64_t FindReplaceBuildingActor()
{
    static uint64_t ReplaceBuildingActor = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"STAT_Fort_BuildingSMActorReplaceBuildingActor", false);

        if (!sRef.Get())
            return ReplaceBuildingActor = Memcury::Scanner::FindPattern("4C 89 44 24 ? 55 56 57 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 45").Get();

        return ReplaceBuildingActor = sRef.ScanFor(VersionInfo.EngineVersion == 4.20 || (VersionInfo.EngineVersion == 4.21 && VersionInfo.FortniteVersion < 6.30) || VersionInfo.EngineVersion >= 4.27 ? std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 } : std::vector<uint8_t>{ 0x4C, 0x8B }, false, 0, 1, 1000).Get();
    }

    return ReplaceBuildingActor;
}

uint64_t FindKickPlayer()
{
    if (VersionInfo.EngineVersion == 4.16)
    {
        auto pattern = Memcury::Scanner::FindPattern("40 53 56 48 81 EC ? ? ? ? 48 8B DA 48 8B F1 E8 ? ? ? ? 48 8B 06 48 8B CE").Get();

        if (pattern)
            return pattern;
    }
    else if (std::floor(VersionInfo.FortniteVersion) == 18)
        return Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8B EC 48 83 EC 60 48 83 65 ? ? 4C 8B F2 83 65 E8 00 4C 8B E1 83 65 EC").Get();
    else if (std::floor(VersionInfo.FortniteVersion) == 19)
    {
        auto pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 45 33 E4 48 8B FA 41 8B DC").Get();

        if (!pattern)
            pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 48 8B EC 48 83 EC 60 48 8B FA 48 8B F1 E8").Get();

        return pattern;
    }
    else if (VersionInfo.EngineVersion >= 5.4)
    {
        auto pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 45 33 ED 48 8B FA 41 8B DD").Get();

        if (!pattern)
            pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 33 DB 48 8B FA 89 5C 24").Get();

        if (!pattern)
            pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 33 FF 48 8B F2 89 7C 24").Get();

        if (!pattern)
            pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 45 33 E4 48 8B F2 41 8B FC").Get();

        if (!pattern)
            pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 45 33 E4 48 8B FA 41 8B DC").Get();

        if (!pattern)
            pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 45 33 F6 48 89 4C 24 ? 45 8B FE").Get();

        return pattern;
    }
    else if (VersionInfo.EngineVersion >= 5.0)
    {
        auto fr = Memcury::Scanner::FindPattern("48 89 5C ?? ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 ?? 45 33 E4 48 8B FA 41 8B DC").Get();

        if (!fr)
            fr = Memcury::Scanner::FindPattern("48 89 5C ?? ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 ?? 45 33 FF 48 8B FA 41 8B DF").Get();

        if (!fr)
            fr = Memcury::Scanner::FindPattern("48 89 5C ?? ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 ?? 45 33 ED 48 8B FA 41 8B DD").Get();

        if (!fr)
            fr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 45 33 ED 48 8B FA 41 8B DD").Get();

        if (!fr)
            fr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 33 DB 48 8B FA").Get();

        return fr;
    }
    else if (VersionInfo.FortniteVersion >= 7.00 && VersionInfo.FortniteVersion <= 15.50)
    {
        return Memcury::Scanner::FindPattern("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 49 8B F0 48 8B DA 48 85 D2").Get();
    }

    auto sRef = Memcury::Scanner::FindStringRef(L"Validation Failure: %s. kicking %s", false, VersionInfo.EngineVersion <= 4.21, VersionInfo.FortniteVersion >= 19).Get();


    if (sRef)
        for (int i = 0; i < 2000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x53)
                return sRef - i;

            else if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x55)
                return sRef - i;
        }

    auto sRef2 = Memcury::Scanner::FindStringRef(L"Failed to kick player").Get();

    if (sRef2)
        for (int i = 0; i < 3000; i++)
        {
            if (*(uint8_t*)(sRef2 - i) == 0x48 && *(uint8_t*)(sRef2 - i + 1) == 0x89 && *(uint8_t*)(sRef2 - i + 2) == 0x5C)
                return sRef2 - i;

            if (VersionInfo.FortniteVersion >= 17)
                if (*(uint8_t*)(sRef2 - i) == 0x48 && *(uint8_t*)(sRef2 - i + 1) == 0x8B && *(uint8_t*)(sRef2 - i + 2) == 0xC4)
                    return sRef2 - i;
        }

    return Memcury::Scanner::FindPattern("40 53 41 56 48 81 EC ? ? ? ? 48 8B 01 48 8B DA 4C 8B F1 FF 90").Get();
}

uint64_t FindEncryptionPatch()
{
    static uint64_t EncryptionPatch = 0;

    if (EncryptionPatch == 0)
    {
        auto EncryptionPatchPoint = Memcury::Scanner::FindPattern("83 BD ? ? ? ? 01 7F 18 49 8D 4D D8 48 8B D6 E8 ? ? ? ? 48", false).Get();

        if (!EncryptionPatchPoint)
            EncryptionPatchPoint = Memcury::Scanner::FindPattern("83 BD ? ? ? ? ? 7F 18 49 8D 4D D8 48 8B D7 E8").Get();
        
        if (!EncryptionPatchPoint)
            EncryptionPatchPoint = Memcury::Scanner::FindPattern("83 7D 88 01 7F 0D 48 8B CE E8", false).Get();

        if (!EncryptionPatchPoint)
            EncryptionPatchPoint = Memcury::Scanner::FindPattern("83 7C 24 ?? 01 7F 0D 48 8B CF E8", false).Get();

        if (!EncryptionPatchPoint)
            EncryptionPatchPoint = Memcury::Scanner::FindPattern("83 7C 24 ? ? 7F 0D 49 8B CE").Get();

        if (!EncryptionPatchPoint)
            EncryptionPatchPoint = Memcury::Scanner::FindPattern("83 7C 24 ? ? 7F 0D 48 8B CE").Get();

        if (!EncryptionPatchPoint)
            EncryptionPatchPoint = Memcury::Scanner::FindPattern("83 7D ? ? 7F 14 48 8D 8B").Get();

        if (EncryptionPatchPoint)
            for (int i = 0; i < 9; i++)
            {
                if (*(uint8_t*)(EncryptionPatchPoint + i) == 0x7F)
                    EncryptionPatch = EncryptionPatchPoint + i;
            }
    }

    return EncryptionPatch;
}

uint64_t FindRemoveInventoryItem()
{
    static uint64_t RemoveInventoryItem = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        std::vector<uint8_t> funcStart = VersionInfo.EngineVersion == 4.16 ? std::vector<uint8_t>{ 0x44, 0x88, 0x4C } : (VersionInfo.FortniteVersion >= 16 && (VersionInfo.FortniteVersion < 20 || VersionInfo.FortniteVersion >= 22) ? std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 } : std::vector<uint8_t>{ 0x48, 0x89, 0x5C });

        auto sRef = FindNameRef(L"ServerRemoveInventoryItem", 0, false);
        uintptr_t uFuncCall = 0;
        for (int i = 0; i < 2000; i++)
        {
            if (VersionInfo.EngineVersion == 4.16)
            {
                if (*(uint8_t*)(sRef - i) == 0x44 && *(uint8_t*)(sRef - i + 1) == 0x88 && *(uint8_t*)(sRef - i + 2) == 0x4C)
                {
                    uFuncCall = sRef - i;
                    break;
                }
            }
            else
            {
                if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                {
                    uFuncCall = sRef - i;
                    break;
                }
                else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                {
                    uFuncCall = sRef - i;
                    break;
                }
            }
        }

        auto ServerRemoveInventoryItemCall = Memcury::Scanner::FindPointerRef((PVOID)uFuncCall, 0, true);

        for (int i = 0; i < 400; i++)
        {
            if (*(uint8_t*)(ServerRemoveInventoryItemCall.Get() - i) == 0x48 && *(uint8_t*)(ServerRemoveInventoryItemCall.Get() - i + 1) == 0x89 && *(uint8_t*)(ServerRemoveInventoryItemCall.Get() - i + 2) == 0x5C)
                return RemoveInventoryItem = ServerRemoveInventoryItemCall.Get() - i;
            else if (*(uint8_t*)(ServerRemoveInventoryItemCall.Get() - i) == 0x48 && *(uint8_t*)(ServerRemoveInventoryItemCall.Get() - i + 1) == 0x83 && *(uint8_t*)(ServerRemoveInventoryItemCall.Get() - i + 2) == 0xEC)
                return RemoveInventoryItem = ServerRemoveInventoryItemCall.Get() - i;
        }
    }

    return RemoveInventoryItem;
}

uint64_t FindRemoveInventoryStateValue()
{
    static uint64_t RemoveInventoryStateValue = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        std::vector<uint8_t> funcStart = VersionInfo.EngineVersion == 4.16 ? std::vector<uint8_t>{ 0x44, 0x88, 0x4C } : (VersionInfo.FortniteVersion >= 16 && (VersionInfo.FortniteVersion < 20 || VersionInfo.FortniteVersion >= 22) ? std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 } : std::vector<uint8_t>{ 0x48, 0x89, 0x5C });

        auto sRef = FindNameRef(L"ServerRemoveInventoryStateValue", 0, false);
        for (int i = 0; i < 2000; i++)
        {
            if (VersionInfo.EngineVersion == 4.16)
            {
                if (*(uint8_t*)(sRef - i) == 0x44 && *(uint8_t*)(sRef - i + 1) == 0x88 && *(uint8_t*)(sRef - i + 2) == 0x4C)
                {
                    RemoveInventoryStateValue = sRef - i;
                    break;
                }
            }
            else
            {
                if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                {
                    RemoveInventoryStateValue = sRef - i;
                    break;
                }
                else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                {
                    RemoveInventoryStateValue = sRef - i;
                    break;
                }
            }
        }
    }

    return RemoveInventoryStateValue;
}

uint64_t FindSetInventoryStateValue()
{
    static uint64_t SetInventoryStateValue = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        std::vector<uint8_t> funcStart = VersionInfo.EngineVersion == 4.16 ? std::vector<uint8_t>{ 0x44, 0x88, 0x4C } : (VersionInfo.FortniteVersion >= 16 && (VersionInfo.FortniteVersion < 20 || VersionInfo.FortniteVersion >= 22) ? std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 } : std::vector<uint8_t>{ 0x48, 0x89, 0x5C });

        auto sRef = FindNameRef(L"ServerSetInventoryStateValue", 0, false);
        for (int i = 0; i < 2000; i++)
        {
            if (VersionInfo.EngineVersion == 4.16)
            {
                if (*(uint8_t*)(sRef - i) == 0x44 && *(uint8_t*)(sRef - i + 1) == 0x88 && *(uint8_t*)(sRef - i + 2) == 0x4C)
                {
                    SetInventoryStateValue = sRef - i;
                    break;
                }
            }
            else
            {
                if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                {
                    SetInventoryStateValue = sRef - i;
                    break;
                }
                else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                {
                    SetInventoryStateValue = sRef - i;
                    break;
                }
            }
        }
    }

    return SetInventoryStateValue;
}

uint64_t FindOnRep_ZiplineState()
{
    static uint64_t OnRep_ZiplineState = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"ZIPLINES!! Role(%s)  AFortPlayerPawn::OnRep_ZiplineState ZiplineState.bIsZiplining=%d", false).Get();
        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"ZIPLINES!! GetLocalRole()(%s)  AFortPlayerPawn::OnRep_ZiplineState ZiplineState.bIsZiplining=%d", false).Get();
        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef("AFortPlayerPawn::HandleZiplineStateChanged").Get();

        if (sRef)
        {
            for (int i = 0; i < 0x400; i++)
            {
                if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x53)
                    return OnRep_ZiplineState = sRef - i;

                if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                    return OnRep_ZiplineState = sRef - i;

                if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                    return OnRep_ZiplineState = sRef - i;
            }
        }
    }

    return OnRep_ZiplineState;
}

uint64 FindGiveAbilityAndActivateOnce()
{
    static uint64_t GiveAbilityAndActivateOnce = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion == 4.26)
        {
            GiveAbilityAndActivateOnce = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 40 49 8B 40 10 49 8B D8 48 8B FA 48 8B F1", false).Get();

            if (!GiveAbilityAndActivateOnce)
                GiveAbilityAndActivateOnce = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 40 49 8B 40 10 49").Get();

            return GiveAbilityAndActivateOnce;
        }

        auto sRef = Memcury::Scanner::FindStringRef(L"GiveAbilityAndActivateOnce called on ability %s on the client, not allowed!", true, 0, VersionInfo.EngineVersion >= 5.0).Get();

        for (int i = 0; i < 1000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x55)
                return GiveAbilityAndActivateOnce = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                return GiveAbilityAndActivateOnce = sRef - i;
        }

        return 0;
    }

    return GiveAbilityAndActivateOnce;
}

uint64_t FindClearAbility()
{
    static uint64_t ClearAbility = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto GiveAbilityAndActivateOnce = FindGiveAbilityAndActivateOnce();

        if (!GiveAbilityAndActivateOnce)
            return 0;

        int skip = 0;
        for (int i = 0; i < 2048; i++)
        {
            // mov rcx, (rdi|rsi)
            // call ClearAbility
            // jmp
            if (
                *(uint8_t*)(GiveAbilityAndActivateOnce + i) == 0x48 && *(uint8_t*)(GiveAbilityAndActivateOnce + i + 1) == 0x8B &&
                *(uint8_t*)(GiveAbilityAndActivateOnce + i + 3) == 0xE8 &&
                (*(uint8_t*)(GiveAbilityAndActivateOnce + i + 8) == 0xE9 || *(uint8_t*)(GiveAbilityAndActivateOnce + i + 8) == 0xEB))
            {
                return ClearAbility = Memcury::Scanner(GiveAbilityAndActivateOnce + i + 3).RelativeOffset(1).Get();
            }
        }
    }

    return ClearAbility;
}

uint64 FindGameSessionPatch()
{
    auto sRef = Memcury::Scanner::FindStringRef(L"Gamephase Step: %s", false).Get();
    uint64 Beginning = 0;

    if (!sRef)
    {
        Beginning = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 20 E8 ? ? ? ? 48 8B D8 48 85 C0 0F 84 ? ? ? ? E8").Get();

        if (!Beginning)
            return 0;
    }
    else
    {
        for (int i = 0; i < 3000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x55)
            {
                Beginning = sRef - i;
                break;
            }
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 &&
                *(uint8_t*)(sRef - i + 2) == 0x5C)
            {
                Beginning = sRef - i;
                break;
            }
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B &&
                *(uint8_t*)(sRef - i + 2) == 0xC4)
            {
                Beginning = sRef - i;
                break;
            }
        }
    }

    if (!Beginning)
        return 0;

    for (int i = 0; i < 500; i++)
        if (*(uint8_t*)(Beginning + i) == 0x0F && *(uint8_t*)(Beginning + i + 1) == 0x84)
            return Beginning + i + 1;

    return 0;
}

uint64 FindRemoveFromAlivePlayers()
{
    static uint64 RemoveFromAlivePlayers = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"FortGameModeAthena: Player [%s] removed from alive players list (Team [%d]).  Player count is now [%d].  Team count is now [%d].", false).Get();

        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"FortGameModeAthena: Player [%s] removed from alive players list (Team [%d]).  Player count is now [%d]. PlayerBots count is now [%d]. Team count is now [%d].", false).Get();

        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"FortGameModeAthena::RemoveFromAlivePlayers: Player [%s] PC [%s] removed from alive players list (Team [%d]).  Player count is now [%d]. PlayerBots count is now [%d]. Team count is now [%d].", true, 0, VersionInfo.FortniteVersion >= 16).Get();

        for (int i = 0; i < 0x1200; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x4C && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x4C)
                return RemoveFromAlivePlayers = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x54)
            {
                for (int z = 3; z < 50; z++)
                    if (*(uint8_t*)(sRef - i - z) == 0x4C && *(uint8_t*)(sRef - i - z + 1) == 0x89 && *(uint8_t*)(sRef - i - z + 2) == 0x4C)
                        return RemoveFromAlivePlayers = sRef - i - z;

                return RemoveFromAlivePlayers = sRef - i;
            }
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                return RemoveFromAlivePlayers = sRef - i;
            else if (VersionInfo.EngineVersion >= 5.3 && *(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                return RemoveFromAlivePlayers = sRef - i;
        }

        return 0;
    }

    return RemoveFromAlivePlayers;
}


uint64 FindStartAircraftPhase()
{
    static uint64_t StartAircraftPhase = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion < 4.27)
        {
            auto sRef = Memcury::Scanner::FindStringRef(L"STARTAIRCRAFT").Get();

            if (!sRef)
                return 0;

            int numCalls = 0;

            for (int i = 0; i < 150; i++)
                if (*(uint8_t*)(sRef + i) == 0xE8)
                    if (++numCalls == 2)
                        return StartAircraftPhase = Memcury::Scanner(sRef + i).RelativeOffset(1).Get();
        }
        else
        {
            auto sRef = Memcury::Scanner::FindStringRef(L"STAT_StartAircraftPhase").Get();

            if (!sRef)
                return 0;

            for (int i = 0; i < 1000; i++)
                if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                    return StartAircraftPhase = sRef - i;
        }

        return 0;
    }

    return StartAircraftPhase;
}

uint64_t FindSetPickupItems()
{
    static uint64_t SetPickupItems = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion == 4.16 || VersionInfo.EngineVersion == 4.19)
            return SetPickupItems = Memcury::Scanner::FindPattern("48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 80 B9 ? ? ? ? ? 41 0F B6 E9").Get();
        else if (VersionInfo.FortniteVersion <= 3.3)
            return SetPickupItems = Memcury::Scanner::FindPattern("48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 80 B9 ? ? ? ? ? 41 0F B6 E9 49 8B F8 48 8B F1 0F 85 ? ? ? ? 48 83 7A").Get();
        else if (VersionInfo.EngineVersion == 4.20)
            return SetPickupItems = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 41 56 48 83 EC 20 80 B9 ? ? ? ? ? 45 0F B6 F1 49 8B E8").Get();
        else if (VersionInfo.EngineVersion == 4.21)
        {
            SetPickupItems = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 57 41 57 48 83 EC 30 80 B9 ? ? ? ? ? 41 0F B6", false).Get();

            if (!SetPickupItems)
                SetPickupItems = Memcury::Scanner::FindPattern("48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 80 B9 ? ? ? ? ? 41 0F B6 E9").Get();

            return SetPickupItems;
        }
        else if (VersionInfo.EngineVersion == 4.22)
            return SetPickupItems = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 41 56 41 57 48 83 EC 30 80 B9 ? ? ? ? ? 45 0F B6 F1").Get();
        else if (VersionInfo.EngineVersion == 4.23)
            return SetPickupItems = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 41 56 41 57 48 83 EC 30 80 B9 ? ? ? ? ? 45 0F B6 F1 4D").Get();
        else if (VersionInfo.EngineVersion == 4.24 || VersionInfo.EngineVersion == 4.25)
            return SetPickupItems = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 57 41 57 48 83 EC ? 80 B9").Get();
        else if (VersionInfo.EngineVersion == 4.26)
        {
            SetPickupItems = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 57 41 57 48 83 EC ? 80 B9").Get();

            if (!SetPickupItems)
                SetPickupItems = Memcury::Scanner::FindPattern("48 89 5C 24 ? 56 57 41 57 48 83 EC ? 80 B9").Get();

            return SetPickupItems;
        }
        else if (VersionInfo.EngineVersion == 4.27)
            return SetPickupItems = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 80 B9 ? ? ? ? ? 45 8A").Get();
        else if (VersionInfo.EngineVersion >= 5.0)
        {
            SetPickupItems = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 80 B9 ? ? ? ? ? 45 8A F9").Get();

            if (!SetPickupItems)
                SetPickupItems = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 80 B9 ? ? ? ? ? 45 8A F1").Get();
        }
    }

    return SetPickupItems;
}

uint64_t FindCallPreReplication()
{
    static uint64_t CallPreReplication = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion == 4.16)
            return CallPreReplication = Memcury::Scanner::FindPattern("48 85 D2 0F 84 ? ? ? ? 48 8B C4 55 57 41 57 48 8D 68 A1 48 81 EC").Get();
        else if (VersionInfo.EngineVersion == 4.19)
            return CallPreReplication = Memcury::Scanner::FindPattern("48 85 D2 0F 84 ? ? ? ? 48 8B C4 55 57 41 54 48 8D 68 A1 48 81 EC ? ? ? ? 48 89 58 08 4C").Get();
        else if (VersionInfo.FortniteVersion >= 2.5 && VersionInfo.FortniteVersion <= 3.3)
            return CallPreReplication = Memcury::Scanner::FindPattern("48 85 D2 0F 84 ? ? ? ? 56 41 56 48 83 EC 38 4C 8B F2").Get();
        else if (std::floor(VersionInfo.FortniteVersion) == 20)
        {
            CallPreReplication = Memcury::Scanner::FindPattern("48 85 D2 0F 84 ? ? ? ? 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 40 F6 41 58 30 48 8B EA 48 8B D9 40 B6 01").Get();

            if (!CallPreReplication)
                CallPreReplication = Memcury::Scanner::FindPattern("48 85 D2 0F 84 ? ? ? ? 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 40 F6 41 58 30 48 8B EA 48 8B D9 75").Get();
        }
        else if (VersionInfo.FortniteVersion < 22)
            return CallPreReplication = Memcury::Scanner::FindPattern("48 85 D2 0F 84 ? ? ? ? 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 40 F6 41 58 30 4C 8B F2").Get();
        else if (VersionInfo.FortniteVersion <= 22.30)
            return CallPreReplication = Memcury::Scanner::FindPattern("48 85 D2 0F 84 ? ? ? ? 48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 4C 89 60 ? 55 41 56 41 57 48 8B EC 48 83 EC ? F6 41 ? ? 4C 8B FA 48 8B").Get();
        else if (VersionInfo.FortniteVersion >= 22.40)
            return CallPreReplication = Memcury::Scanner::FindPattern("48 85 D2 0F 84 ? ? ? ? 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 56 41 57 48 8B EC 48 83 EC ? F6 41").Get();
    }

    return CallPreReplication;
}

uint64_t FindSendClientAdjustment()
{
    static uint64_t SendClientAdjustment = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion >= 5.4)
        {
            SendClientAdjustment = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B D8 48 85 C0 74 ? 80 78").Get();

            if (!SendClientAdjustment)
                SendClientAdjustment = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 8B 91 ? ? ? ? 48 8B D9 83 FA").Get();
        }
        else if (VersionInfo.FortniteVersion >= 25.00)
        {
            SendClientAdjustment = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B D9 E8 ? ? ? ? 80 B8 ? ? ? ? ? 75 ? 8B 93").Get();
            
            if (!SendClientAdjustment)
                SendClientAdjustment = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 83 3D ? ? ? ? ? 48 8B D9 75").Get();
        }
        else if (VersionInfo.FortniteVersion >= 23.00)
        {
            SendClientAdjustment = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 83 3D ? ? ? ? ? 48 8B D9 75").Get();

            if (!SendClientAdjustment)
                SendClientAdjustment = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 83 EC ? 83 3D ? ? ? ? ? 48 8B F1").Get();
        }
        else if (VersionInfo.FortniteVersion >= 20.00)
            SendClientAdjustment = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 8B 91 ? ? ? ? 48 8B D9 83 FA").Get();
        else
            SendClientAdjustment = Memcury::Scanner::FindPattern("40 53 48 83 EC 20 48 8B 99 ? ? ? ? 48 39 99 ? ? ? ? 74 0A 48 83 B9", false).Get();
    }

    return SendClientAdjustment;
}

uint64 FindSetChannelActor()
{
    static uint64_t SetChannelActor = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion == 4.16)
            return SetChannelActor = Memcury::Scanner::FindPattern("4C 8B DC 55 53 57 41 54 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 45 33").Get();
        else if (VersionInfo.FortniteVersion == 3.3)
            return SetChannelActor = Memcury::Scanner::FindPattern("48 8B C4 55 53 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 89 70 10 48 8B D9 48 89 78 18 48 8D 35").Get();
        else if (VersionInfo.EngineVersion >= 4.19 && VersionInfo.FortniteVersion < 3.3)
        {
            SetChannelActor = Memcury::Scanner::FindPattern("48 8B C4 55 53 57 41 54 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 45 33 E4 48 89 70", false).Get();

            if (!SetChannelActor)
                return SetChannelActor = Memcury::Scanner::FindPattern("48 8B C4 55 53 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 89 70 E8 48 8B D9").Get();
        }
        else if (std::floor(VersionInfo.FortniteVersion) == 20)
            return SetChannelActor = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 45 33 E4 48 8D 3D ? ? ? ? 44 89 A5").Get();
        else if (VersionInfo.FortniteVersion >= 27)
            return SetChannelActor = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 45 33 FF 48 8D 35").Get();
        else if (VersionInfo.FortniteVersion >= 21)
        {
            SetChannelActor = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 33 FF 4C 8D 35 ? ? ? ? 89 BD").Get();

            if (!SetChannelActor)
                SetChannelActor = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 45 33 F6").Get();

            if (!SetChannelActor)
                SetChannelActor = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 45 33 ED 48 8D 35 ? ? ? ? 44 89 AD ? ? ? ? 48 8B D9 48 8B 41 28 45 8B E0 48 8B FA 45 8B").Get();

            if (!SetChannelActor)
                SetChannelActor = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 45 33 FF").Get();
        }
    }

    return SetChannelActor;
}


uint64 FindSetChannelActorForDestroy()
{
    static uint64_t SetChannelActorForDestroy = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"SetChannelActorForDestroy: Channel %d. NetGUID <%s> Path: %s. Bits: %d", false, 0, VersionInfo.FortniteVersion >= 19, true).Get();

        if (!sRef)
            return SetChannelActorForDestroy = 0;

        for (int i = 0; i < 2000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x55)
                return SetChannelActorForDestroy = sRef - i;
        }
    }

    return SetChannelActorForDestroy;
}


uint64 FindSendDestructionInfo()
{
    static uint64_t SendDestructionInfo = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"STAT_NetSendDestructionInfo").Get();

        if (!sRef)
            return SendDestructionInfo = 0;

        for (int i = 0; i < 2000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x4c)
            {
                if (*(uint8_t*)(sRef - i - 5) == 0x48 && *(uint8_t*)(sRef - i - 4) == 0x89 && *(uint8_t*)(sRef - i - 3) == 0x5c)
                    i += 5;

                return SendDestructionInfo = sRef - i;
            }
        }
    }

    return SendDestructionInfo;
}

uint64 FindCreateChannel()
{
    static uint64_t CreateChannel = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.FortniteVersion <= 3.3)
            return CreateChannel = Memcury::Scanner::FindPattern("40 56 57 41 54 41 55 41 57 48 83 EC 60 48 8B 01 41 8B F9 45 0F B6 E0").Get();
        else if (VersionInfo.FortniteVersion >= 20)
        {
            auto sRef = Memcury::Scanner::FindStringRef(L"STAT_NetConnection_CreateChannelByName").Get();

            if (!sRef)
                return CreateChannel = 0;

            for (int i = 0; i < 2000; i++)
            {
                if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5c)
                    return CreateChannel = sRef - i;
                else if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x55)
                    return CreateChannel = sRef - i;
            }
        }
    }

    return CreateChannel;
}

uint64 FindReplicateActor()
{
    static uint64_t ReplicateActor = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion == 4.16)
            return ReplicateActor = Memcury::Scanner::FindPattern("40 55 53 57 41 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8D 59 68 4C 8B F1 48 8B").Get();
        else if (VersionInfo.FortniteVersion == 3.3)
            return ReplicateActor = Memcury::Scanner::FindPattern("48 8B C4 55 53 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 A8 0F 29 78 98 48 89 70 E8 4C").Get();
        else if (VersionInfo.EngineVersion >= 4.19 && VersionInfo.FortniteVersion <= 3.2)
        {
            ReplicateActor = Memcury::Scanner::FindPattern("40 55 56 57 41 54 41 55 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 4C", false).Get();

            if (!ReplicateActor)
                ReplicateActor = Memcury::Scanner::FindPattern("40 55 56 41 54 41 55 41 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 4C 8B E9 48 8B 49 68 48").Get();
        }
        else if (VersionInfo.FortniteVersion >= 20)
        {
            auto sRef = Memcury::Scanner::FindStringRef(L"STAT_NetReplicateActorTime").Get();

            for (int i = 0; i < 2000; i++)
            {
                if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x55)
                    return ReplicateActor = sRef - i;
                else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                    return ReplicateActor = sRef - i;
            }
        }
    }

    return ReplicateActor;
}

uint64 FindCloseActorChannel()
{
    static uint64_t CloseActorChannel = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"UActorChannel::Close: ChIndex: %d, Actor: %s", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"UActorChannel::Close: ChIndex: %d, Actor: %s, Reason: %s", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 2000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                return CloseActorChannel = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                return CloseActorChannel = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x55)
                return CloseActorChannel = sRef - i;
        }
    }

    return CloseActorChannel;
}

uint64 FindClientHasInitializedLevelFor()
{
    static uint64_t ClientHasInitializedLevelFor = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        ClientHasInitializedLevelFor = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B 5A 20 48 8B F1 4C 8B C3 48 8D", false).Get();

        if (!ClientHasInitializedLevelFor)
            ClientHasInitializedLevelFor = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B 5A 20 48 8B F1 4C 8B C3", false).Get();

        if (!ClientHasInitializedLevelFor)
            ClientHasInitializedLevelFor = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B F9 48 85 D2 74 35 48").Get();

        if (!ClientHasInitializedLevelFor)
            ClientHasInitializedLevelFor = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 20 48 8B DA 48 8B F9 E8 ? ? ? ? 48 8B D0 48 8B CB E8 ? ? ? ? 48 8B C8").Get();

        if (!ClientHasInitializedLevelFor)
            ClientHasInitializedLevelFor = Memcury::Scanner::FindPattern("40 53 48 83 EC ? 48 8B D9 48 8B CA E8 ? ? ? ? 48 8B C8 E8 ? ? ? ? 4C 8D 9B").Get();

        if (!ClientHasInitializedLevelFor)
            ClientHasInitializedLevelFor = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 20 8B 81 ? ? ? ? 48 8B F9 48 8B 5A ? 3B 81").Get();
    }

    return ClientHasInitializedLevelFor;
}


uint64 FindStartBecomingDormant()
{
    static uint64_t StartBecomingDormant = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"StartBecomingDormant: %s", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 2000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                return StartBecomingDormant = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                return StartBecomingDormant = sRef - i;
        }
    }

    return StartBecomingDormant;
}



uint64 FindFlushDormancy()
{
    static uint64_t FlushDormancy = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"FlushDormancy: %s. Connection: %s", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 2000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x40 && *(uint8_t*)(sRef - i + 1) == 0x55)
                return FlushDormancy = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x4C && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xDC)
                return FlushDormancy = sRef - i;
        }
    }

    return FlushDormancy;
}

uint64_t FindEnterAircraft()
{
    static uint64_t EnterAircraft = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"EnterAircraft: [%s] is attempting to enter aircraft after having already exited.", true, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 1000; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            if (*Ptr == 0x48 && (*(Ptr + 1) == 0x83 || *(Ptr + 1) == 0x81) && *(Ptr + 2) == 0xEC)
            {
                sRef = uint64_t(Ptr);
                break;
            }
            else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8D && *(Ptr + 2) == 0xAC)
            {
                sRef = uint64_t(Ptr);
                break;
            }
        }
        for (int i = 0; i < 1000; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            if (*(uint8_t*)(sRef - i) == 0x40 && (*(uint8_t*)(sRef - i + 1) == 0x53 || *(uint8_t*)(sRef - i + 1) == 0x55))
            {
                EnterAircraft = uint64_t(Ptr);
                break;
            }
            else if (VersionInfo.FortniteVersion >= 15 && *Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c)
            {
                EnterAircraft = uint64_t(Ptr);
                break;
            }
            else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x74)
            {
                EnterAircraft = uint64_t(Ptr);
                break;
            }
        }
    }

    return EnterAircraft;
}

uint64_t FindGetPlayerViewPoint()
{
    uint64 ftspAddr = 0;

    auto ftspRef = Memcury::Scanner::FindStringRef(L"%s failed to spawn a pawn", true, 0, VersionInfo.FortniteVersion >= 19).Get();

    for (int i = 0; i < 1000; i++)
    {
        if (*(uint8_t*)(ftspRef - i) == 0x40 && *(uint8_t*)(ftspRef - i + 1) == 0x53)
        {
            ftspAddr = ftspRef - i;
            break;
        }
        else if (*(uint8_t*)(ftspRef - i) == 0x48 && *(uint8_t*)(ftspRef - i + 1) == 0x89 && *(uint8_t*)(ftspRef - i + 2) == 0x5C)
        {
            ftspAddr = ftspRef - i;
            break;
        }
    }

    if (!ftspAddr)
        return 0;

    auto PCVft = AFortPlayerControllerAthena::GetDefaultObj()->Vft;
    int ftspIdx = 0;

    for (int i = 0; i < 0x1000; i++)
    {
        if (PCVft[i] == (void*)ftspAddr)
        {
            ftspIdx = i;
            break;
        }
    }

    if (ftspIdx == 0)
        return 0;

    if (VersionInfo.FortniteVersion >= 20 && *(uint8_t*)(PCVft[ftspIdx - 1]) == 0xE9)
        return Memcury::Scanner(PCVft[ftspIdx - 1]).RelativeOffset(1).Get();

    return __int64(PCVft[ftspIdx - 1]);
}

uint32_t FindOnItemInstanceAddedVft()
{
    static uint32_t OnItemInstanceAddedVft = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        uint64_t OnItemInstanceAdded = 0;

        auto inFunc = Memcury::Scanner::FindPattern("41 57 48 83 EC ? 48 8B 01 4C 8B F2 48 8B F1 FF 90 ? ? ? ? 4C 8B F8").Get();

        if (!inFunc)
            inFunc = Memcury::Scanner::FindPattern("41 56 48 83 EC ? 48 8B 01 4C 8B F2 48 8B F1 FF 90 ? ? ? ? 48 8B F8").Get();

        if (!inFunc)
            return 0;

        for (int i = 0; i < 1000; i++)
        {
            if (*(uint8_t*)(inFunc - i) == 0x48 && *(uint8_t*)(inFunc - i + 1) == 0x8B && *(uint8_t*)(inFunc - i + 2) == 0xC4)
            {
                OnItemInstanceAdded = inFunc - i;
                break;
            }
            else if (*(uint8_t*)(inFunc - i) == 0x48 && *(uint8_t*)(inFunc - i + 1) == 0x89 && *(uint8_t*)(inFunc - i + 2) == 0x74)
            {
                if (*(uint8_t*)(inFunc - i - 5) == 0x48 && *(uint8_t*)(inFunc - i - 4) == 0x89 && *(uint8_t*)(inFunc - i - 3) == 0x5C)
                    i += 5;

                OnItemInstanceAdded = inFunc - i;
                break;
            }
        }

        auto ItemDefObj = UFortItem::GetDefaultObj();

        for (int i = 0; i < 0x400; i++)
            if (uint64_t(ItemDefObj->Vft[i]) == OnItemInstanceAdded)
                return OnItemInstanceAddedVft = i;
    }

    return OnItemInstanceAddedVft;
}

uint64_t FindGetNamePool()
{
    static uint64_t GetNamePool = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        GetNamePool = Memcury::Scanner::FindPattern("48 83 EC ? 80 3D ? ? ? ? ? 0F 84 ? ? ? ? 48 8D 05 ? ? ? ? 48 83 C4 ? C3").Get();

        if (!GetNamePool)
            GetNamePool = Memcury::Scanner::FindPattern("48 83 EC ? 80 3D ? ? ? ? ? 74 ? 48 8D 05 ? ? ? ? 48 83 C4 ? C3 48 8D 0D").Get();
    }

    return GetNamePool;
}


uint64_t FindIsNetReady()
{
    static uint64_t IsNetReady = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto IsNetReady_ = Memcury::Scanner::FindPattern("84 D2 74 ? 8B 81 ? ? ? ? F7 D8");

        if (VersionInfo.FortniteVersion >= 20 && IsNetReady_.Get())
        {
            for (int i = 0; i < 0x30; i++)
            {
                if (*(uint8_t*)(IsNetReady_.Get() - i) == 0x48 && *(uint8_t*)(IsNetReady_.Get() - i + 1) == 0x83 && *(uint8_t*)(IsNetReady_.Get() - i + 2) == 0xEC)
                    return IsNetReady = IsNetReady_.Get() - i;
            }
            return IsNetReady = IsNetReady_.Get();
        }
        else
            IsNetReady = IsNetReady_.Get();
    }

    return IsNetReady;
}

uint64_t FindSpawnInitialSafeZone()
{
    static uint64_t SpawnInitialSafeZone = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"FortGameModeAthena::SpawnInitialSafeZone bShouldSpawnSafeZoneIndicator == false, skipping spawning Safe Zone Indicator", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 2000; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c)
            {
                SpawnInitialSafeZone = uint64_t(Ptr);
                break;
            }
            else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8b && *(Ptr + 2) == 0xc4)
            {
                SpawnInitialSafeZone = uint64_t(Ptr);
                break;
            }
        }
    }

    return SpawnInitialSafeZone;
}


uint64_t FindUpdateSafeZonesPhase()
{
    static uint64_t UpdateSafeZonesPhase = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef("AFortGameModeAthena::UpdateSafeZonesPhase").Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 2000; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c)
            {
                UpdateSafeZonesPhase = uint64_t(Ptr);
                break;
            }
            else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8b && *(Ptr + 2) == 0xc4)
            {
                UpdateSafeZonesPhase = uint64_t(Ptr);
                break;
            }
        }
    }

    return UpdateSafeZonesPhase;
}

uint64 FindUpdateIrisReplicationViews()
{
    static uint64_t UpdateIrisReplicationViews = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        UpdateIrisReplicationViews = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 8B F1 E8 ? ? ? ? BB").Get();

        if (!UpdateIrisReplicationViews)
            UpdateIrisReplicationViews = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 0F 10 0D").Get();
    }

    return UpdateIrisReplicationViews;
}

uint64 FindPreSendUpdate()
{
    static uint64_t PreSendUpdate = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef("ReplicationSystem_PreSendUpdate");
        if (sRef.IsValid())
            return PreSendUpdate = sRef.ScanFor({ 0x48, 0x89, 0x5C }, false).Get();

        PreSendUpdate = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 8B 02 48 8B F9 4C 8B 41").Get();

        if (!PreSendUpdate)
            PreSendUpdate = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 48 8B EC 48 83 EC ? 8B 02 48 8B F9").Get();

        if (!PreSendUpdate)
            PreSendUpdate = Memcury::Scanner::FindPattern("4C 8B DC 49 89 4B ? 55").Get();
    }

    return PreSendUpdate;
}

uint64_t FindHandleMatchHasStarted()
{
    static uint64_t HandleMatchHasStarted = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"AFortGameModeAthena::HandleMatchHasStarted: NumPlayers: %i", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"AFortGameModeAthena::HandleMatchHasStarted: Playlist: %s, NumPlayers: %i", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 2000; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            if (*Ptr == 0x48 && *(Ptr + 1) == 0x8b && *(Ptr + 2) == 0xc4)
            {
                HandleMatchHasStarted = uint64_t(Ptr);
                break;
            }
        }
    }

    return HandleMatchHasStarted;
}

uint64_t FindInitializeBuildingActor()
{
    static uint64_t InitializeBuildingActor = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto sRef = Memcury::Scanner::FindStringRef(L"STAT_Fort_BuildingSMActorInitializeBuildingActor", false);

        if (!sRef.Get())
            return 0;

        return InitializeBuildingActor = sRef.ScanFor(std::vector<uint8_t>{ 0x48, 0x8B, 0xC4 }, false, 0, 1, 1000).Get();
    }

    return InitializeBuildingActor;
}

uint64_t FindPostInitializeSpawnedBuildingActor()
{
    static uint64_t PostInitializeSpawnedBuildingActor = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        auto PCVft = ABuildingSMActor::GetDefaultObj()->Vft;
        int ibaIdx = 0;
        auto InitializeBuildingActor = FindInitializeBuildingActor();

        for (int i = 0; i < 2000; i++)
        {
            if (PCVft[i] == (void*)InitializeBuildingActor)
            {
                ibaIdx = i;
                break;
            }
        }

        if (ibaIdx == 0)
            return 0;

        return PostInitializeSpawnedBuildingActor = __int64(PCVft[ibaIdx + 1]);
    }

    return PostInitializeSpawnedBuildingActor;
}

uint64 FindInitializeFlightPath()
{
    static uint64_t InitializeFlightPath = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        InitializeFlightPath = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B E9 41 8A D9").Get();
        
        if (!InitializeFlightPath)
            InitializeFlightPath = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B E9 41 0F B6 D9").Get();
    }

    return InitializeFlightPath;
}

uint64 FindReset()
{
    static uint64_t Reset = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        Reset = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC ? 48 8B 91 ? ? ? ? 48 8B F9 48 85 D2 74 ? 48 8B 01").Get();
    }

    return Reset;
}

uint64_t FindNotifyGameMemberAdded()
{
    static uint64_t NotifyGameMemberAdded = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion >= 5.1)
        {
            NotifyGameMemberAdded = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 88 54 24").Get();

            if (!NotifyGameMemberAdded)
                NotifyGameMemberAdded = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 88 54 24").Get();

            if (!NotifyGameMemberAdded)
                NotifyGameMemberAdded = Memcury::Scanner::FindPattern("40 55 53 56 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 88 54 24").Get();

            return NotifyGameMemberAdded;
        }

        auto sRef = Memcury::Scanner::FindStringRef(L"%s: Adding Player state with UniqueId: %s, in team: %d, and in squad: %d", false, 0, VersionInfo.FortniteVersion >= 19).Get();

        if (!sRef)
            return 0;


        for (int i = 0; i < 2000; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            if (*Ptr == 0x40 && *(Ptr + 1) == 0x55)
            {
                NotifyGameMemberAdded = uint64_t(Ptr);
                break;
            }
        }
    }

    return NotifyGameMemberAdded;
}


uint64 FindSetGamePhase()
{
    static uint64_t SetGamePhase = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        SetGamePhase = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B 81 ? ? ? ? 4C 8B E9 44 0F B6 E2").Get();
    }

    return SetGamePhase;
}

uint64_t FindPayBuildableClassPlacementCost()
{
    auto sRef = Memcury::Scanner::FindStringRef(L"Failed to remove item %s during pay building costs, item duplicated!", false, VersionInfo.FortniteVersion >= 10.30 && VersionInfo.FortniteVersion < 20, VersionInfo.FortniteVersion >= 18.00);

    if (VersionInfo.FortniteVersion >= 10.00)
    {
        auto Addr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8B EC 48 81 EC ? ? ? ? 48 8B 1A 33 FF");

        //if (!Addr.IsValid())
            //Addr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 56 41 57 48 8B EC 48 83 EC ? 48 8B 1A 33 FF");

        if (Addr.IsValid())
            return Addr.Get();
    }

    if (sRef.IsValid())
    {
        for (int i = 0; i < 2000; i++)
        {
            auto Ptr = (uint8_t*)(sRef.Get() - i);

            if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
                return uint64_t(Ptr);
            else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
                return uint64_t(Ptr);
        }
    }

    return Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC ? 48 8B 1A 33 F6").Get();
}

uint64_t FindCanAffordToPlaceBuildableClass()
{
    auto sRef = Memcury::Scanner::FindStringRef(L"Resource not found! Resource Type is %i, might be invalid", false, 0, VersionInfo.FortniteVersion >= 18.00);

    if (VersionInfo.FortniteVersion >= 18)
    {
        auto Addr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC ? 33 DB 4C 8B F2 48 8B F9 48 39 1A");

        if (!Addr.IsValid())
            Addr = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 56 48 83 EC ? 33 DB 4C 8B F2 48 8B F9 48 39 1A");

        if (Addr.IsValid())
            return Addr.Get();
    }

    if (sRef.IsValid())
    {
        if (VersionInfo.FortniteVersion < 12.00)
        {
            for (int i = 0; i < 2000; i++)
            {
                auto Ptr = (uint8_t*)(sRef.Get() - i);

                if (*Ptr == 0x40 && (*(Ptr + 1) == 0x57 || *(Ptr + 1) == 0x55 || *(Ptr + 1) == 0x53))
                    return uint64_t(Ptr);
            }
        }
        else
            for (int i = 0; i < 2000; i++)
            {
                auto Ptr = (uint8_t*)(sRef.Get() - i);

                if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
                    return uint64_t(Ptr);
                else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
                    return uint64_t(Ptr);
            }
    }
    else
        return Memcury::Scanner::FindPattern("40 53 56 41 56 48 83 EC ? 48 8B 1A 4C 8B F2").Get();
    return 0;
}

uint64 FindCanPlaceBuildableClassInStructuralGrid()
{
    static uint64_t CanPlaceBuildableClassInStructuralGrid = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        CanPlaceBuildableClassInStructuralGrid = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 54 41 56 41 57 48 83 EC ? 4D 8B E1 4D 8B F8 48 8B F2").Get();

        if (!CanPlaceBuildableClassInStructuralGrid)
            CanPlaceBuildableClassInStructuralGrid = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 48 8B 1A 4D 8B E9").Get();
    }

    return CanPlaceBuildableClassInStructuralGrid;
}


uint64 FindLoadPlayset(const std::vector<uint8_t>& Bytes, int recursive)
{
    static uint64_t LoadPlayset = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.EngineVersion >= 5.0)
        {
            LoadPlayset = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 4C 8B B1 ? ? ? ? 45").Get();

            if (!LoadPlayset)
                LoadPlayset = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 4C 8B A9").Get();

            if (!LoadPlayset)
                LoadPlayset = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 4C 8B B9").Get();

            if (!LoadPlayset)
                LoadPlayset = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 55 41 54 41 55 41 56 41 57 48 8D 68 ? 48 81 EC ? ? ? ? 4C 8B A9").Get();
            
            if (!LoadPlayset)
                LoadPlayset = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 55 57 41 54 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 4C 8B A1").Get();

            return LoadPlayset;
        }

        if (recursive >= 2)
            return 0;

        auto StringRef = Memcury::Scanner::FindStringRef(L"UPlaysetLevelStreamComponent::LoadPlayset Error: no owner for %s", 0, 1);

        if (!StringRef.Get())
            return 0;

        for (int i = 0; i < 400; i++)
        {
            auto CurrentByte = *(uint8_t*)(StringRef.Get() - i);

            if (CurrentByte == Bytes[0])
            {
                bool Found = true;
                for (int j = 1; j < Bytes.size(); j++)
                    if (*(uint8_t*)(StringRef.Get() - i + j) != Bytes[j])
                    {
                        Found = false;
                        break;
                    }

                if (Found)
                    return LoadPlayset = StringRef.Get() - i;
            }

            if (CurrentByte == 0xC3)
                return FindLoadPlayset({ 0x40, 0x55 }, ++recursive);
        }

        return 0;
    }

    return LoadPlayset;
}

uint32 FindSpawnDecoVft()
{
    auto sRef = Memcury::Scanner::FindStringRef(L"AFortTrapTool::SpawnDeco World is tearing down.  Early-ing out.", false, 0, VersionInfo.FortniteVersion >= 19);

    uint64 SpawnDeco = 0;
    for (int i = 0; i < 2000; i++)
    {
        auto Ptr = (uint8_t*)(sRef.Get() - i);

        if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
        {
            SpawnDeco = uint64_t(Ptr);
            break;
        }
        else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
        {
            SpawnDeco = uint64_t(Ptr);
            break;
        }
    }

    auto ActorVft = DefaultObjImpl("FortTrapTool")->Vft;

    for (int i = 0; i < 0x500; i++)
    {
        if (ActorVft[i] == (void*)SpawnDeco)
        {
            return i;
        }
    }
    return 0;
}

uint32 FindShouldAllowServerSpawnDecoVft()
{
    auto sRef = Memcury::Scanner::FindStringRef(L"Tried to place deco item %s %s that isn't actually in player inventory!", false, 0, VersionInfo.FortniteVersion >= 19, false);

    uint64 ShouldAllowServerSpawnDecoPart = 0;

    for (int i = 0; i < 2000; i++)
    {
        auto Ptr = (uint8_t*)(sRef.Get() - i);

        if (*Ptr == 0x48 && *(Ptr + 1) == 0x83 && *(Ptr + 2) == 0xEC)
        {
            ShouldAllowServerSpawnDecoPart = uint64_t(Ptr);
            break;
        }
        else if (*Ptr == 0x48 && *(Ptr + 1) == 0x81 && *(Ptr + 2) == 0xEC)
        {
            ShouldAllowServerSpawnDecoPart = uint64_t(Ptr);
            break;
        }
    }

    uint64 ShouldAllowServerSpawnDeco = 0;
    for (int i = 0; i < 2000; i++)
    {
        auto Ptr = (uint8_t*)(ShouldAllowServerSpawnDecoPart - i);

        if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
        {
            ShouldAllowServerSpawnDeco = uint64_t(Ptr);
            break;
        }
        else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
        {
            ShouldAllowServerSpawnDeco = uint64_t(Ptr);
            break;
        }
    }

    auto ActorVft = DefaultObjImpl("FortDecoTool")->Vft;

    for (int i = 0; i < 0x500; i++)
    {
        if (ActorVft[i] == (void*)ShouldAllowServerSpawnDeco)
        {
            return i;
        }
    }
    return 0;
}

uint64 FindSetState()
{
    auto sRef = Memcury::Scanner::FindStringRef(L"Time from Setup to InProgress: %6.2fms", false, 0, VersionInfo.FortniteVersion >= 19, false);

    if (!sRef.IsValid())
        return 0;

    for (int i = 0; i < 2000; i++)
    {
        auto Ptr = (uint8_t*)(sRef.Get() - i);

        if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
            return uint64_t(Ptr);
        else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
            return uint64_t(Ptr);
    }

    return 0;
}

uint64 FindMinigameSettingsBuilding__BeginPlay()
{
    return Memcury::Scanner::FindPattern("40 55 57 41 56 41 57 48 8B EC 48 83 EC ? 80 3D").Get();
}

uint64_t FindPickSupplyDropLocation()
{
    auto sRef = Memcury::Scanner::FindStringRef(L"PickSupplyDropLocation: Failed to find valid location using rejection.  Using safe zone location.", false, 0, VersionInfo.FortniteVersion >= 19, false);

    if (!sRef.IsValid())
        sRef = Memcury::Scanner::FindStringRef("AFortAthenaMapInfo::PickSupplyDropLocation");

    for (int i = 0; i < 2000; i++)
    {
        auto Ptr = (uint8_t*)(sRef.Get() - i);

        if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
            return uint64_t(Ptr);
        else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
            return uint64_t(Ptr);
    }

    return 0;
}

uint64_t FindSetPickupTarget()
{
    auto sRef = Memcury::Scanner::FindStringRef(L"%s: Attempted to spawn non-world item %s!", false, 0, VersionInfo.FortniteVersion >= 17, false);

    if (!sRef.IsValid())
        sRef = Memcury::Scanner::FindStringRef(L"Attempted to spawn non-world item %s!", false, 0, VersionInfo.FortniteVersion >= 17, false);

    for (int i = 0; i < 0x1500; i++)
    {
        auto Ptr = (uint8_t*)(sRef.Get() - i);

        if (*Ptr == 0x40 && (*(Ptr + 1) == 0x53 || *(Ptr + 1) == 0x55))
            return uint64_t(Ptr);
        else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
            return uint64_t(Ptr);
        else if (*Ptr == 0x4C && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xDC)
            return uint64_t(Ptr);
        //else if (VersionInfo.FortniteVersion >= 15 && *Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
        //    return uint64_t(Ptr);
    }

    return 0;
}

uint64 FindInitializePlayerGameplayAbilities()
{
    auto sRef = Memcury::Scanner::FindStringRef(L"InitializePlayerGameplayAbilities with invalid PlayerStateOrProxy!", false, 0, VersionInfo.FortniteVersion >= 19, false);

    if (!sRef.IsValid())
        return 0;

    for (int i = 0; i < 2000; i++)
    {
        auto Ptr = (uint8_t*)(sRef.Get() - i);

        if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
            return uint64_t(Ptr);
        else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
            return uint64_t(Ptr);
        else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x4C)
            return uint64_t(Ptr);
        else if (*Ptr == 0x40 && *(Ptr + 1) == 0x57)
            return uint64_t(Ptr);
    }

    return 0;
}

uint64 FindListenCall()
{
    static uint64_t ListenCall = 0;
    static bool bInitialized = false;

    if (!bInitialized)
    {
        bInitialized = true;

        if (VersionInfo.FortniteVersion <= 6 || VersionInfo.FortniteVersion > 16)
            return ListenCall = 0;

        auto sRef = Memcury::Scanner::FindStringRef(L"LoadMap: failed to Listen(%s)");

        if (!sRef.IsValid())
            return ListenCall = 0;

        for (int i = 0; i < 0x100; i++)
        {
            auto Ptr = (uint8_t*)(sRef.Get() - i);

            if (*Ptr == 0xE8 && *(Ptr + 5) == 0x84 && *(Ptr + 6) == 0xC0)
                return ListenCall = uint64_t(Ptr);
        }

        return 0;
    }

    return ListenCall;
}

void FindNullsAndRetTrues()
{
    if (VersionInfo.EngineVersion == 4.16)
    {
        //NullFuncs.push_back(Memcury::Scanner::FindPattern("4C 89 44 24 ? 88 54 24 ? 48 89 4C 24 ? 56 57 48 81 EC ? ? ? ? 33 C0 83 F8 ? 0F 84 ? ? ? ? B8").Get());
        NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 54 24 ? 48 89 4C 24 ? 55 53 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 8B 41 08 C1 E8 05").Get());
        //NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 54 24 ? 48 89 4C 24 ? 55 53 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 8B 41 ? C1 E8 ? A8 ? 0F 84 ? ? ? ? 80 3D").Get());
    }
    else if (VersionInfo.EngineVersion == 4.20)
    {
        NullFuncs.push_back(Memcury::Scanner::FindStringRef(L"Widget Class %s - Running Initialize On Archetype, %s.").ScanFor(std::vector<uint8_t>{ 0x48, 0x89, 0x54 }, false).Get());
        if (VersionInfo.FortniteVersion > 3.2)
        {
            //if (VersionInfo.FortniteVersion == 4.1) 
            //    NullFuncs.push_back(Memcury::Scanner::FindPattern("4C 8B DC 55 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 49 89 5B 10 48 8D 05 ? ? ? ? 48 8B 1D ? ? ? ? 49 89 73 18 33 F6 40").Get());

            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 57 48 81 EC ? ? ? ? 4C 8B 82 ? ? ? ? 48 8B F9 0F 29 70 E8 0F 29 78 D8").Get());
            //NullFuncs.push_back(Memcury::Scanner::FindPattern("4C 8B DC 55 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 49 89 5B 10 48 8D 05 ? ? ? ? 48 8B 1D ? ? ? ? 49 89 73 18 33 F6 40").Get());
        }
        //else
        //    NullFuncs.push_back(Memcury::Scanner::FindPattern("E8 ? ? ? ? F0 FF 0D ? ? ? ? 0F B6 C3").RelativeOffset(1).Get());
    }
    else if (VersionInfo.EngineVersion == 4.21)
    {
        NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 70 10 57 48 81 EC ? ? ? ? 48 8B BA ? ? ? ? 48 8B DA 0F 29").Get());
        NullFuncs.push_back(Memcury::Scanner::FindStringRef(L"Widget Class %s - Running Initialize On Archetype, %s.").ScanFor(VersionInfo.FortniteVersion < 6.3 ? std::vector<uint8_t>{ 0x40, 0x55 } : std::vector<uint8_t>{ 0x48, 0x89, 0x5C }, false).Get());
        NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 41 0F B6 F0 48 8D 15 ? ? ? ? 48 8B F9 41 B8").Get());
    }
    else if (VersionInfo.EngineVersion >= 4.22 && VersionInfo.FortniteVersion <= 12.00)
    {
        if (VersionInfo.EngineVersion == 4.24)
            NullFuncs.push_back(Memcury::Scanner::FindPattern("40 53 57 48 83 EC ? 48 8B 01 48 8B F9 FF 90 ? ? ? ? 48 8B C8").Get());
        //else if (VersionInfo.EngineVersion == 4.25)
        //    NullFuncs.push_back(Memcury::Scanner::FindPattern("40 57 41 56 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F B6 FA 44 8B F1 74 3A 80 3D ? ? ? ? ? 0F 82").Get());
        NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 30 48 8B 41 28 48 8B DA 48 8B F9 48 85 C0 74 34 48 8B 4B 08 48 8D").Get());
    }
    else if (VersionInfo.EngineVersion == 4.25)
    {
        //NullFuncs.push_back(Memcury::Scanner::FindStringRef(L"Widget Class %s - Running Initialize On Archetype, %s.").ScanFor(std::vector<uint8_t>{ 0x48, 0x89, 0x5C }, false).Get()); // for 12.41
        //NullFuncs.push_back(Memcury::Scanner::FindPattern(VersionInfo.FortniteVersion == 12.41 ? "40 57 41 56 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F B6 FA 44 8B F1 74 3A 80 3D ? ? ? ? ? 0F" : "40 57 41 56 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F B6 FA 44 8B F1 74 3A 80 3D ? ? ? ? ? 0F 82").Get());
        //if (VersionInfo.FortniteVersion == 12.41)
        //    NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B FA 4C 8B F9").Get());
        /*else */if (VersionInfo.FortniteVersion == 12.61)
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 20 4C 8B A5").Get());
    }
    else if (VersionInfo.FortniteVersion == 14.60)
        NullFuncs.push_back(Memcury::Scanner::FindPattern("40 55 57 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F B6 FA 44 8B F9 74 3B 80 3D ? ? ? ? ? 0F").Get());
    else if (VersionInfo.FortniteVersion >= 17.00)
    {
        //NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 10 48 89 6C 24 20 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B F9").Get());
        if (std::floor(VersionInfo.FortniteVersion) == 17)
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 70 08 48 89 78 10 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC ? ? ? ? 45 33 ED").Get());
        else if (VersionInfo.FortniteVersion >= 19.00 && VersionInfo.FortniteVersion < 20.00)
        {
            auto p = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 55 41 56 48 8B EC 48 83 EC 50 83 65 28 00 40 B6 05 40 38 35 ? ? ? ? 4C").Get();

            if (!p)
                p = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8B EC 48 83 EC ? 83 65 ? ? 40 B6").Get();

            NullFuncs.push_back(p);
        }
        else if (VersionInfo.FortniteVersion >= 20.00 && VersionInfo.EngineVersion == 5.0)
        {
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 4C 89 40 18 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 98 48 81 EC ? ? ? ? 49 8B 48 20 45 33").Get());
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 20 48 8B 41 20 48 8B FA 48 8B D9 BA ? ? ? ? 83 78 08 03 0F 8D").Get());
            NullFuncs.push_back(Memcury::Scanner::FindPattern("4C 89 44 24 ? 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 68 48 8D 05 ? ? ? ? 0F").Get());
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 48 8B F9 48 8B CA E8").Get());
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 ? 41 ? 48 83 EC 60 45 33 F6 4C 8D ? ? ? ? ? 48 8B DA").Get());
        }
        else if (VersionInfo.EngineVersion == 5.1)
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4D 8B F1").Get());
        else if (VersionInfo.EngineVersion == 5.2)
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 0F 29 70 ? 48 8D 6C 24 ? 48 83 E5 ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 8B F2").Get());
        else if (VersionInfo.EngineVersion == 5.3)
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 55 57 41 54 41 56 41 57 48 81 EC").Get());
        else if (VersionInfo.EngineVersion >= 5.4)
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 ? 48 89 70 ? 55 57 41 54 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 0F 29 70 ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 45 33 FF").Get());
    }

    if (VersionInfo.FortniteVersion == 2.5)
        NullFuncs.push_back(Memcury::Scanner::FindPattern("40 55 56 41 56 48 8B EC 48 81 EC ? ? ? ? 48 8B 01 4C 8B F2").Get());
    else if (VersionInfo.EngineVersion == 5.0)
    {
        auto pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 50 4C 8B FA 48 8B F1 E8").Get();

        if (!pattern)
            pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 55 41 56 48 8B EC 48 83 EC ? 4C 8B F2").Get();

        NullFuncs.push_back(pattern);
    }
    else if (VersionInfo.EngineVersion == 5.1 || VersionInfo.EngineVersion == 5.2)
    {
        auto pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC ? 4C 8B E2 4C 8B F1 E8 ? ? ? ? 48 8B 0D").Get();

        if (!pattern)
            pattern = Memcury::Scanner::FindPattern("48 89 5C ? ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC ? 4C 8B E2 48 8B F1").Get();

        NullFuncs.push_back(pattern);
    }
    else if (VersionInfo.EngineVersion >= 5.3)
    {
        auto pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 4C 8B FA 48 8B F1 E8 ? ? ? ? 48 8B 0D").Get();

        if (!pattern)
            pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B F2 4C 8B F1 E8 ? ? ? ? 48 8B 0D").Get();

        NullFuncs.push_back(pattern);
    }
    else if (VersionInfo.EngineVersion == 4.27)
    {
        auto pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 70 4C 8B FA 4C").Get();

        if (!pattern)
            pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC ? 4C 8B FA 4C 8B F1 E8 ? ? ? ? 48 8B 0D").Get();

        NullFuncs.push_back(pattern);
    }
    else
    {
        auto sRef = Memcury::Scanner::FindStringRef(L"Changing GameSessionId from '%s' to '%s'", true, 0, VersionInfo.FortniteVersion >= 19);
        NullFuncs.push_back(sRef.ScanFor(VersionInfo.EngineVersion >= 4.27 ? std::vector<uint8>{ 0x48, 0x89, 0x5C } : std::vector<uint8>{ 0x40, 0x55 }, false, 0, 1, 2000).Get());
    }

    //if (VersionInfo.EngineVersion == 4.23)
    //    NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 0F B6 FA").Get());
    /*else if (VersionInfo.EngineVersion >= 4.24 && VersionInfo.EngineVersion < 4.27)
    {
        auto sRef = Memcury::Scanner::FindStringRef(L"STAT_CollectGarbageInternal").Get();
        uint64_t CollectGarbage = 0;

        if (!sRef)
            sRef = Memcury::Scanner::FindStringRef(L"CollectGarbageInternal() is flushing async loading").Get();

        if (sRef)
        {
            for (int i = 0; i < 1000; i++)
    {
                auto Ptr = (uint8_t*)(sRef - i);

                if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
    {
                    CollectGarbage = uint64_t(Ptr);
                    break;
                }
                else if (*Ptr == 0x40 && *(Ptr + 1) == 0x55)
    {
                    CollectGarbage = uint64_t(Ptr);
                    break;
                }
                else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
    {
                    CollectGarbage = uint64_t(Ptr);
                    break;
                }
            }
            NullFuncs.push_back(CollectGarbage);
        }
    }*/

    if (VersionInfo.FortniteVersion == 1.10 || VersionInfo.FortniteVersion == 1.11 || (VersionInfo.FortniteVersion >= 2.2 && VersionInfo.FortniteVersion <= 2.4))
        RetTrueFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 57 41 56 41 57 48 81 EC ? ? ? ? 48 8B 01 49 8B E9 45 0F B6 F8").Get());
    else if (VersionInfo.EngineVersion >= 4.26)
    {
        if (std::floor(VersionInfo.FortniteVersion) == 17)
        {
            auto pattern = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8B EC 48 83 EC 60 4D 8B F9 41 8A F0 4C 8B F2 48 8B F9 45 32 E4").Get();

            if (pattern)
                RetTrueFuncs.push_back(pattern);
            else
                RetTrueFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8B EC 48 83 EC 60 49 8B D9 45 8A").Get());
        }
        else
            RetTrueFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8B EC 48 83 EC 60 49 8B D9 45 8A").Get());
    }
    RetTrueFuncs.push_back(FindKickPlayer());

    if (VersionInfo.FortniteVersion >= 23)
    {
        //NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC ? 4C 8B E2 4C 8B F1").Get());
        //RetTrueFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 8B F0 48 8B DA 48 85 D2 74 ? 48 83 BA").Get());
    }

    if (VersionInfo.FortniteVersion >= 16.00)
    {
        auto RequestExitWithStatus = Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 40 41 B9 ? ? ? ? 0F B6 F9 44 38 0D ? ? ? ? 0F B6 DA 72 24 89 5C 24 30 48 8D 05 ? ? ? ? 89 7C 24 28 4C 8D 05 ? ? ? ? 33 D2 48 89 44 24 ? 33 C9 E8 ? ? ? ?").Get();
        if (!RequestExitWithStatus)
            RequestExitWithStatus = Memcury::Scanner::FindPattern("4C 8B DC 49 89 5B 08 49 89 6B 10 49 89 73 18 49 89 7B 20 41 56 48 83 EC 30 80 3D ? ? ? ? ? 49 8B").Get();
        if (!RequestExitWithStatus)
            RequestExitWithStatus = Memcury::Scanner::FindPattern("48 8B C4 48 89 58 18 88 50 10 88 48 08 57 48 83 EC 30").Get();

        if (!RequestExitWithStatus)
        {
            auto RequestExit = Memcury::Scanner::FindPattern("40 53 48 83 EC ? 80 3D ? ? ? ? ? 0F B6 D9 72 ? 48 8D 05 ? ? ? ? 89 5C 24 ? 41 B9 ? ? ? ? 48 89 44 24 ? 4C 8D 05 ? ? ? ? 33 D2 33 C9 E8 ? ? ? ? 48 8D 0D").Get();
            if (!RequestExit)
                RequestExit = Memcury::Scanner::FindPattern("88 4C 24 ? 53 48 83 EC ? 80 3D ? ? ? ? ? 8A D9").Get();
            if (!RequestExit)
                RequestExit = Memcury::Scanner::FindPattern("40 53 48 83 EC ? 41 B9 ? ? ? ? 0F B6 D9").Get();
            if (!RequestExit)
                RequestExit = Memcury::Scanner::FindPattern("40 53 48 83 EC ? 80 3D ? ? ? ? ? 0F B6 D9").Get();
            if (!RequestExit)
                RequestExit = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 33 DB 0F B6 F9").Get();

            if (RequestExit)
                NullFuncs.push_back(RequestExit);
        }
        else
            NullFuncs.push_back(RequestExitWithStatus);
    }

    if (VersionInfo.EngineVersion >= 4.21)
    {
        if (VersionInfo.EngineVersion == 4.21 || VersionInfo.EngineVersion == 4.22)
            RetTrueFuncs.push_back(Memcury::Scanner::FindPattern("4C 89 4C 24 20 55 56 57 41 56 48 8D 6C 24 D1").Get());
        else
        {
            auto sRef = Memcury::Scanner::FindStringRef(L"CanActivateAbility %s failed, blueprint refused", true, 0, VersionInfo.EngineVersion >= 5.0).Get();

            if (!sRef)
                sRef = Memcury::Scanner::FindStringRef(L"CanActivateAbility %s failed, called with invalid Handle", true, 0, VersionInfo.EngineVersion >= 5.0).Get();

            if (sRef)
            {
                for (int i = 0; i < 0x2000; i++)
                {
                    auto Ptr = (uint8_t*)(sRef - i);

                    if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
                    {
                        RetTrueFuncs.push_back(uint64_t(Ptr));
                        break;
                    }
                    else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
                    {
                        RetTrueFuncs.push_back(uint64_t(Ptr));
                        break;
                    }
                }
            }
        }
    }

    auto sRef = Memcury::Scanner::FindStringRef(L"AFortPlayerControllerAthena::HasStreamingLevelsCompletedLoadingUnLoading(): %s still not visible", false, 0, VersionInfo.FortniteVersion >= 19).Get();

    if (sRef)
    {
        for (int i = 0; i < 1000; i++)
        {
            auto Ptr = (uint8_t*)(sRef - i);

            if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C)
            {
                RetTrueFuncs.push_back(uint64_t(Ptr));
                break;
            }
            else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4)
            {
                RetTrueFuncs.push_back(uint64_t(Ptr));
                break;
            }
        }
    }

    if (VersionInfo.FortniteVersion >= 23)
    {
        NullFuncs.push_back(Memcury::Scanner::FindStringRef(L"STAT_FortCurieVoxelFirePropagationManager_IgniteGrassInBounds").ScanFor({ 0x48, 0x8B, 0xC4 }, false).Get());
    }

    if (VersionInfo.EngineVersion < 5.0)
    {
        auto CanCreateInCurrentContext = Memcury::Scanner::FindPattern("8B ? E8 ? ? ? ? 84 C0 75 ? 80 3D ? ? ? ? 03 0F 82 ? ? ? ? ? 8B ? 18 ? 8D 54");

        if (!CanCreateInCurrentContext.IsValid())
            CanCreateInCurrentContext = Memcury::Scanner::FindPattern("8B ? E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 80 3D ? ? ? ? 03 0F 82 ? ? ? ? ? 8B ? 18 ? 8D 54");

        if (!CanCreateInCurrentContext.IsValid())
            CanCreateInCurrentContext = Memcury::Scanner::FindPattern("8B ? E8 ? ? ? ? 84 C0 75 ? 80 3D ? ? ? ? 03 72 ? ? 8B ? 18 ? 8D 54");

        if (CanCreateInCurrentContext.IsValid())
            RetTrueFuncs.push_back(CanCreateInCurrentContext.RelativeOffset(3).Get());
    }
    else
    {
        // ue5.1+ i think, they inlined the VFT call
    }
}
