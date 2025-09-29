#pragma once
#include "../../pch.h"

inline uint64_t FindGIsClient()
{
    static uintptr_t GIsClient = 0;

    if (GIsClient == 0)
    {
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
        for (int i = 0; i < 0x100; i++) {
            auto Ptr = (uint8_t*)(sRef - i);

            for (auto& Load : GIsClientLoads) {
                bool bMatches = true;
                for (int x = 0; x < Load.size(); x++) {
                    if (Load[x] != *(Ptr + x)) {
                        bMatches = false;
                        break;
                    }
                }
                if (!bMatches) continue;
                if ((Load[0] == 0x44 || Load[0] == 0x40) && *(Ptr + 2) == 0x74) continue;
                if (!correctByte)
                    correctByte = Load[0];
                else if (Load[0] != correctByte)
                    continue;
                if (Skip > 0) {
                    Skip--;
                    continue;
                }

                return GIsClient = Memcury::Scanner(Ptr).RelativeOffset((Load[0] == 0x44 || Load[0] == 0x40) ? 3 : 2, Load[0] == 0xc6).Get();
            }
        }
    }

    return GIsClient;
}

inline uint64_t FindGIsServer()
{
    static uintptr_t GIsServer = 0;

    if (GIsServer == 0)
    {
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
        for (int i = 0; i < 0x100; i++) {
            auto Ptr = (uint8_t*)(sRef - i);

            for (auto& Load : GIsServerLoads) {
                bool bMatches = true;
                for (int x = 0; x < Load.size(); x++) {
                    if (Load[x] != *(Ptr + x)) {
                        bMatches = false;
                        break;
                    }
                }
                if (!bMatches) continue;
                if ((Load[0] == 0x44 || Load[0] == 0x40) && *(Ptr + 2) == 0x74) continue;
                if (!correctByte)
                    correctByte = Load[0];
                else if (Load[0] != correctByte)
                    continue;
                if (Skip > 0) {
                    Skip--;
                    continue;
                }

                return GIsServer = Memcury::Scanner(Ptr).RelativeOffset((Load[0] == 0x44 || Load[0] == 0x40) ? 3 : 2, Load[0] == 0xc6).Get();
            }
        }
    }

    return GIsServer;
}

inline uint64_t FindGetNetMode()
{
    static uint64_t GetNetMode = 0;

    if (GetNetMode == 0)
    {
        GetNetMode = floor(VersionInfo.FortniteVersion) == 18 ? Memcury::Scanner::FindPattern("48 83 EC 28 48 83 79 ? ? 75 20 48 8B 91 ? ? ? ? 48 85 D2 74 1E 48 8B 02 48 8B CA FF 90").Get() : 0;
        if (floor(VersionInfo.FortniteVersion) != 18) {
            auto sRef = Memcury::Scanner::FindStringRef(L"PREPHYSBONES").Get();

            for (int i = 0; i < 1000; i++) {
                auto Ptr = (uint8_t*)(sRef - i);

                if (*Ptr == 0x40 && *(Ptr + 1) == 0x55) {
                    GetNetMode = uint64_t(Ptr);
                    break;
                }
            }

            for (int i = 0; i < 400; i++) {
                auto Ptr = (uint8_t*)(GetNetMode + i);

                if (*Ptr == 0xe8 && *(Ptr - 1) != 0x8b && *(Ptr - 1) != 0xe7) {
                    GetNetMode = uint64_t(Ptr);
                    break;
                }
            }

            GetNetMode = Memcury::Scanner(GetNetMode).RelativeOffset(1).Get();
        }
    }
    
    return GetNetMode;
}

inline uint64_t FindGetWorldContext()
{
    static uint64_t GetWorldContext = 0;

    if (GetWorldContext == 0)
    {
        GetWorldContext = Memcury::Scanner::FindPattern("48 8B 81 ? ? ? ? 48 63 89 ? ? ? ? 4C 8D 04 C8 49 3B C0 74 ? 48 8B 08 48 39 91 ? ? ? ? 75 ? 48 8B C1 C3").Get();
    }

    return GetWorldContext;
}

inline uint64_t FindCreateNetDriver()
{
    static uint64_t CreateNetDriver = 0;

    if (CreateNetDriver == 0)
    {
        CreateNetDriver = Memcury::Scanner::FindPattern("49 8B D8 48 8B F9 E8 ?? ?? ?? ?? 48 8B D0 4C 8B C3 48 8B CF 48 8B 5C 24 ?? 48 83 C4 ?? 5F E9 ?? ?? ?? ??").Get();
        if (!CreateNetDriver) 
        {
            CreateNetDriver = Memcury::Scanner::FindPattern("E8 ?? ?? ?? ?? 4C 8B 44 24 ?? 48 8B D0 48 8B CB E8 ?? ?? ?? ?? 48 83 C4 ?? 5B C3").Get();
            if (!CreateNetDriver) 
                CreateNetDriver = Memcury::Scanner::FindPattern("33 D2 E8 ?? ?? ?? ?? 48 8B D0 4C 8B C3 48 8B CF E8 ?? ?? ?? ?? 48 8B 5C 24 ?? 48 83 C4 ?? 5F C3").Get();
        }

        if (CreateNetDriver) 
        {
            for (int i = 0; i < 0x50; i++) 
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

inline uint64_t FindCreateNetDriverWorldContext()
{
    static uint64_t CreateNetDriver = 0;

    if (CreateNetDriver == 0)
    {
        auto Ptr = Memcury::Scanner::FindPattern("C7 44 24 ? 00 20 00 00 33 D2 48 8B C8 E8 ? ? ? ? 48 ?? 4C");

        if (!Ptr.Get())
            return 0;

        auto scuffness = Ptr.ScanFor({ 0xC3 }, false).ScanFor({ 0x48 }); // should land us right at the start of CreateNetDriver

        return CreateNetDriver = scuffness.Get();
    }

    return CreateNetDriver;
}

inline uint64_t FindInitListen()
{
    static uint64_t InitListen = 0;

    if (InitListen == 0)
    {
        if (VersionInfo.EngineVersion >= 5.0)
            InitListen = Memcury::Scanner::FindPattern("4D 8B C8 4C 8B C2 33 D2 FF 90 ? ? ? ? 84 C0 75 ? 80 3D").ScanFor({ 0x4C, 0x8B, 0xDC }, false).Get();
        else if (VersionInfo.EngineVersion >= 4.27)
            InitListen = Memcury::Scanner::FindPattern("4C 8B DC 49 89 5B 08 49 89 73 10 57 48 83 EC 50 48 8B BC 24 ? ? ? ? 49 8B F0 48 8B 01 48 8B").Get();
        else {
            auto sRef = Memcury::Scanner::FindStringRef(L"%s IpNetDriver listening on port %i").Get();
            int skip = 1;
            for (int i = 0; i < 2000; i++) {
                auto Ptr = (uint8_t*)(sRef - i);

                if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c) {
                    if (skip > 0) {
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

inline uint64_t FindSetWorld()
{
    static uint64_t SetWorld = 0;

    if (SetWorld == 0)
    {
        SetWorld = VersionInfo.FortniteVersion < 13 ? Memcury::Scanner::FindStringRef(L"AOnlineBeaconHost::InitHost failed").ScanFor({ 0x48, 0x8B, 0xD0, 0xE8 }, false).RelativeOffset(4).Get() : 0;
        
        if (VersionInfo.FortniteVersion >= 13) 
        {
            auto Season = (int)floor(VersionInfo.FortniteVersion);
            uint32 VftIdx = 0;
            switch (Season) {
            case 13:
                VftIdx = 0x70;
                break;
            case 18:
                VftIdx = 0x73;
                break;
            case 19:
                VftIdx = 0x7a;
                break;
            case 20:
                VftIdx = 0x7b;
                break;
            default:
                if (VersionInfo.FortniteVersion >= 14 && VersionInfo.FortniteVersion <= 15.2) VftIdx = 0x71;
                else if (VersionInfo.FortniteVersion >= 15.3 && VersionInfo.FortniteVersion < 18) VftIdx = 0x72;
                else
                    VftIdx = 0x7c;
                break;
            }
            SetWorld = uintptr_t(DefaultObjImpl("NetDriver")->Vft[VftIdx]);
        }
    }

    return SetWorld;
}

inline uint64_t FindTickFlush()
{
    static uint64_t TickFlush = 0;

    if (TickFlush == 0)
    {
        TickFlush = VersionInfo.EngineVersion == 4.16 ? Memcury::Scanner::FindPattern("4C 8B DC 55 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 45 0F 29 43 ? 45 0F 29 4B ? 48 8B 05 ? ? ? ? 48").Get() : 0;
        
        if (VersionInfo.EngineVersion == 4.19) 
            TickFlush = Memcury::Scanner::FindPattern("4C 8B DC 55 53 56 57 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 41 0F 29 7B").Get();

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
                for (int i = 0; i < 1000; i++) {
                    auto Ptr = (uint8_t*)(sRef - i);

                    if (*Ptr == 0x48 && *(Ptr + 1) == 0x8b && *(Ptr + 2) == 0xc4) {
                        TickFlush = uint64_t(Ptr);
                        break;
                    }
                    else if (*Ptr == 0x4c && *(Ptr + 1) == 0x8b && *(Ptr + 2) == 0xdc) {
                        TickFlush = uint64_t(Ptr);
                        break;
                    }
                    else if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c) {
                        TickFlush = uint64_t(Ptr);
                        break;
                    }
                }
        }
    }

    return TickFlush;
}

inline uint64_t FindServerReplicateActors()
{
    static uint64_t ServerReplicateActors = 0;

    if (ServerReplicateActors == 0)
    {
        int ServerReplicateActorsVft = 0;
        switch ((int)floor(VersionInfo.FortniteVersion)) {
        case 4:
            ServerReplicateActorsVft = 0x53;
            break;
        case 5:
            ServerReplicateActorsVft = 0x54;
            break;
        case 10:
        case 9:
        case 8:
        case 7:
            if (VersionInfo.FortniteVersion >= 7.40 && VersionInfo.FortniteVersion <= 8.40)
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
        else 
        {
            MessageBoxA(nullptr, "This version is currently not supported.", "Erbium", MB_ICONERROR);
            ExitProcess(0);
        }
    }
    return ServerReplicateActors;
}

inline uint64_t FindSendRequestNow()
{
    static uint64_t SendRequestNow = 0;

    if (SendRequestNow == 0)
    {
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

inline uint64 FindGetMaxTickRate()
{
    static uint64_t GetMaxTickRate = 0;

    if (GetMaxTickRate == 0)
    {
        if (VersionInfo.EngineVersion >= 5.0) 
            return Memcury::Scanner::FindPattern("40 53 48 83 EC 50 0F 29 74 24 ? 48 8B D9 0F 29 7C 24 ? 0F 28 F9 44 0F 29").Get();

        if (VersionInfo.EngineVersion >= 4.27)
            return Memcury::Scanner::FindPattern("40 53 48 83 EC 60 0F 29 74 24 ? 48 8B D9 0F 29 7C 24 ? 0F 28").Get();

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

inline uint64_t FindGiveAbility()
{
    static uint64_t GiveAbility = 0;

    if (GiveAbility == 0)
    {
        if (VersionInfo.EngineVersion <= 4.20)
            return GiveAbility = Memcury::Scanner::FindPattern("48 89 5C 24 ? 56 57 41 56 48 83 EC 20 83 B9").Get();
        else if (VersionInfo.EngineVersion == 4.21)
            return GiveAbility = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 7C 24 ? 41 56 48 83 EC 20 83 B9 ? ? ? ? ? 49 8B E8 4C 8B F2").Get();
        if (VersionInfo.EngineVersion == 5.0)
            return GiveAbility = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC 20 8B 81 ? ? ? ? 49 8B E8 4C").Get();

        Memcury::Scanner addr = Memcury::Scanner::FindStringRef(L"GiveAbilityAndActivateOnce called on ability %s on the client, not allowed!", true, 1, VersionInfo.EngineVersion >= 5.0);

        GiveAbility = addr.ScanFor({ 0xE8 }).RelativeOffset(1).Get();
    }

    return GiveAbility;
}

inline uint64_t FindConstructAbilitySpec()
{
    static uint64_t ConstructAbilitySpec = 0;

    if (ConstructAbilitySpec == 0)
    {
        if (VersionInfo.EngineVersion >= 4.20 && VersionInfo.EngineVersion <= 4.24)
            return ConstructAbilitySpec = Memcury::Scanner::FindPattern("48 89 5C 24 ? 56 57 41 56 48 83 EC 20 83 B9").Get();
        else if (VersionInfo.EngineVersion == 4.25)
        {
            ConstructAbilitySpec = Memcury::Scanner::FindPattern("48 8B 44 24 ? 80 61 29 F8 80 61 31 FE 48 89 41 20 33 C0 89 41", false).Get();

            if (!ConstructAbilitySpec)
                ConstructAbilitySpec = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 7C 24 ? 41 56 48 83 EC 20 83 B9 ? ? ? ? ? 49 8B E8 4C 8B F2").Get();
        }
        else if (VersionInfo.EngineVersion == 4.26)
            return ConstructAbilitySpec = Memcury::Scanner::FindPattern("80 61 31 FE 0F 57 C0 80 61 29 F0 48 8B 44 24 ? 48").Get();
        else if (VersionInfo.EngineVersion == 4.27)
            return ConstructAbilitySpec = Memcury::Scanner::FindPattern("80 61 31 FE 41 83 C9 FF 80 61 29 F0 48 8B 44 24 ? 48 89 41").Get();
        else if (VersionInfo.EngineVersion == 5.0)
            return ConstructAbilitySpec = Memcury::Scanner::FindPattern("4C 8B C9 48 8B 44 24 ? 83 C9 FF 41 80 61 ? ? 41 80 61 ? ? 49 89 41 20 33 C0 41 88 41 30 49 89 41").Get();
    }

    return ConstructAbilitySpec;
}


inline uint64_t FindInternalTryActivateAbility()
{
    static uint64_t InternalTryActivateAbility = 0;
    
    if (InternalTryActivateAbility == 0)
    {
        auto sRef = Memcury::Scanner::FindStringRef(L"InternalTryActivateAbility called with invalid Handle! ASC: %s. AvatarActor: %s", true, 0, VersionInfo.FortniteVersion >= 16).Get();

        for (int i = 0; i < 1000; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                return InternalTryActivateAbility = sRef - i;

            else if (*(uint8_t*)(sRef - i) == 0x4C && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x4C)
                return InternalTryActivateAbility = sRef - i;
        }
    }

    return InternalTryActivateAbility;
}


inline uint64 FindApplyCharacterCustomization()
{
    static uint64_t ApplyCharacterCustomization = 0;

    if (ApplyCharacterCustomization == 0)
    {
        auto sRef = Memcury::Scanner::FindStringRef(L"AFortPlayerState::ApplyCharacterCustomization - Failed initialization, using default parts. Player Controller: %s PlayerState: %s, HeroId: %s", false, 0, VersionInfo.FortniteVersion >= 19, true).Get();

        if (!sRef)
            return 0;

        for (int i = 0; i < 7000; i++)
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
        }

        ApplyCharacterCustomization = Memcury::Scanner::FindPattern("48 8B C4 48 89 50 10 55 57 48 8D 68 A1 48 81 EC ? ? ? ? 80 B9").Get();
    }

    return ApplyCharacterCustomization;
}

inline uint64 FindHandlePostSafeZonePhaseChanged()
{
    static uint64_t HandlePostSafeZonePhaseChanged = 0;

    if (HandlePostSafeZonePhaseChanged == 0)
    {
        if (VersionInfo.EngineVersion == 4.19)
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
            else if (VersionInfo.FortniteVersion < 8 && *(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x5C)
                return HandlePostSafeZonePhaseChanged = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                return HandlePostSafeZonePhaseChanged = sRef - i;
        }

        return 0;
    }

    return HandlePostSafeZonePhaseChanged;
}


inline uint64_t FindStaticFindObject()
{
    static uint64_t StaticFindObject = 0;

    if (StaticFindObject == 0)
    {
        if (VersionInfo.EngineVersion >= 5.0) 
        {
            StaticFindObject = Memcury::Scanner::FindPattern("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 45 33 F6 4C 8B E1 45 0F B6 E9 49 8B F8 41 8B C6", false).Get();
            
            if (!StaticFindObject)
            {
                StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 4C 89 64 24 ? 55 41 55 41 57 48 8B EC 48 83 EC 60 45 8A E1 4C 8B E9 48 83 FA").Get();

                if (!StaticFindObject)
                {
                    StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 4C 89 64 24 ? 55 41 55 41 57 48 8B EC 48 83 EC 50 4C 8B E9").Get();

                    if (!StaticFindObject)
                        StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 7C 24 ? 4C 89 64 24 ? 55 41 56 41 57 48 8B EC 48 83 EC 60 33 FF 4C 8B E1 48 8D 4D E8 45 8A").Get();
                }
            }
        }
        else if (VersionInfo.EngineVersion >= 4.27) 
        {
            if (floor(VersionInfo.FortniteVersion) == 18)
                StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 45 33 ED 45 8A F9 44 38 2D ? ? ? ? 49 8B F8 48 8B").Get();
            else if (VersionInfo.FortniteVersion == 16.50)
                StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60 45 33 ED 45 8A F9 44 38 2D ? ? ? ? 49 8B F8 48 8B F2 4C 8B E1").Get();
            else
                StaticFindObject = Memcury::Scanner::FindPattern("40 55 53 57 41 54 41 55 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85").Get();
        }
        else if (VersionInfo.EngineVersion == 4.16)
            StaticFindObject = Memcury::Scanner::FindPattern("4C 8B DC 57 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 49 89 6B F0 49 89 73 E8").Get();
        else if (VersionInfo.EngineVersion == 4.19)
        {
            StaticFindObject = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8B EC 48 83 EC 60 80 3D ? ? ? ? ? 45 0F B6 F1 49 8B F8").Get();
            
            if (!StaticFindObject)
                StaticFindObject = Memcury::Scanner::FindPattern("4C 8B DC 49 89 5B 08 49 89 6B 18 49 89 73 20 57 41 56 41 57 48 83 EC 60 80 3D").Get();
        }
        else {
            auto sRef = Memcury::Scanner::FindStringRef(L"Illegal call to StaticFindObject() while serializing object data!", false, 1).Get();

            for (int i = 0; i < 1000; i++) 
            {
                auto Ptr = (uint8_t*)(sRef - i);

                if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c) 
                {
                    StaticFindObject = uint64_t(Ptr);
                    break;
                }
            }
        }
    }

    return StaticFindObject;
}

inline uint64_t FindStaticLoadObject()
{
    static uint64_t StaticLoadObject = 0;
    
    if (StaticLoadObject == 0)
    {
        auto sRef = Memcury::Scanner::FindStringRef(L"STAT_LoadObject", false).Get();

        if (!sRef)
        {
            auto sRef2 = Memcury::Scanner::FindStringRef(L"Calling StaticLoadObject during PostLoad may result in hitches during streaming.");
            return sRef2.ScanFor({ 0x40, 0x55 }, false).Get();
        }

        for (int i = 0; i < 400; i++)
        {
            if (*(uint8_t*)(sRef - i) == 0x4C && *(uint8_t*)(sRef - i + 1) == 0x89 && *(uint8_t*)(sRef - i + 2) == 0x4C)
                return StaticLoadObject = sRef - i;
            else if (*(uint8_t*)(sRef - i) == 0x48 && *(uint8_t*)(sRef - i + 1) == 0x8B && *(uint8_t*)(sRef - i + 2) == 0xC4)
                return StaticLoadObject = sRef - i;
        }
    }

    return StaticLoadObject;
}


inline uint64_t FindPickTeam()
{
    static uint64_t PickTeam = 0;

    if (PickTeam == 0)
    {
        if (VersionInfo.EngineVersion == 4.26)
        {
            PickTeam = Memcury::Scanner::FindPattern("88 54 24 10 53 56 41 54 41 55 41 56 48 83 EC 60 4C 8B A1", false).Get();

            if (!PickTeam)
                PickTeam = Memcury::Scanner::FindPattern("88 54 24 10 53 55 56 41 55 41 ? 48 83 EC 70 48", false).Get();

            if (PickTeam)
                return PickTeam;
        }
        else if (VersionInfo.EngineVersion == 5.0)
            return PickTeam = Memcury::Scanner::FindPattern("48 89 5C 24 ? 88 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 70 45 33 ED 4D").Get();
        else if (VersionInfo.EngineVersion == 4.27)
            return PickTeam = Memcury::Scanner::FindPattern("48 89 5C 24 ? 88 54 24 10 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 70 4C 8B A1").Get();
        else if (VersionInfo.FortniteVersion == 7.20 || VersionInfo.FortniteVersion == 7.30)
            return PickTeam = Memcury::Scanner::FindPattern("89 54 24 10 53 56 41 54 41 55 41 56 48 81 EC").Get();

        auto Addr = Memcury::Scanner::FindStringRef(L"PickTeam for [%s] used beacon value [%d]");

        if (!Addr.Get())
            Addr = Memcury::Scanner::FindStringRef(L"PickTeam for [%s] used beacon value [%s]");

        PickTeam = Addr.ScanFor(VersionInfo.FortniteVersion <= 4.1 ? std::vector<uint8_t>{ 0x48, 0x89, 0x6C } : std::vector<uint8_t>{ 0x40, 0x55 }, false, 0, 1, 1000).Get();
    }
    
    return PickTeam;
}

inline uint64_t FindCantBuild()
{
    static uint64_t CantBuild = 0;

    if (CantBuild == 0)
    {
        CantBuild = Memcury::Scanner::FindPattern("48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 41 56 48 83 EC ? 49 8B E9 4D 8B F0", false).Get();

        if (!CantBuild)
            CantBuild = Memcury::Scanner::FindPattern("48 89 54 24 ? 55 56 41 56 48 83 EC 50", false).Get();

        if (!CantBuild)
            CantBuild = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 60 4D 8B F1 4D 8B F8", false).Get();

        if (!CantBuild)
            CantBuild = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 60 49 8B E9 4D 8B F8 48 8B DA 48 8B F9 BE ? ? ? ? 48", false).Get(); 

        if (!CantBuild)
            CantBuild = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 70 49 8B E9 4D 8B F8 48 8B DA 48 8B F9").Get();
    }

    return CantBuild;
}

inline uint64_t FindKickPlayer()
{
    if (VersionInfo.EngineVersion == 4.16)
        return Memcury::Scanner::FindPattern("40 53 56 48 81 EC ? ? ? ? 48 8B DA 48 8B F1 E8 ? ? ? ? 48 8B 06 48 8B CE").Get();

    if (std::floor(VersionInfo.FortniteVersion) == 18)
        return Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8B EC 48 83 EC 60 48 83 65 ? ? 4C 8B F2 83 65 E8 00 4C 8B E1 83 65 EC").Get();

    if (std::floor(VersionInfo.FortniteVersion) == 19)
        return Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 48 8B EC 48 83 EC 60 48 8B FA 48 8B F1 E8").Get();

    if (VersionInfo.FortniteVersion >= 7.00 && VersionInfo.FortniteVersion <= 15.50) {
        return Memcury::Scanner::FindPattern("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 49 8B F0 48 8B DA 48 85 D2").Get();
    }

    auto Addr = Memcury::Scanner::FindStringRef(L"Validation Failure: %s. kicking %s", false, 0, VersionInfo.FortniteVersion >= 19).Get();


    if (Addr) 
        for (int i = 0; i < 2000; i++) 
        {
            if (*(uint8_t*)(Addr - i) == 0x40 && *(uint8_t*)(Addr - i + 1) == 0x53)
                return Addr - i;

            else if (*(uint8_t*)(Addr - i) == 0x40 && *(uint8_t*)(Addr - i + 1) == 0x55)
                return Addr - i;
        }

    auto Addr2 = Memcury::Scanner::FindStringRef(L"Failed to kick player");
    auto Addrr = Addr2.Get();

    if (Addrr) 
        for (int i = 0; i < 3000; i++) {
            if (*(uint8_t*)(Addrr - i) == 0x48 && *(uint8_t*)(Addrr - i + 1) == 0x89 && *(uint8_t*)(Addrr - i + 2) == 0x5C)
                return Addrr - i;

            if (VersionInfo.FortniteVersion >= 17)
                if (*(uint8_t*)(Addrr - i) == 0x48 && *(uint8_t*)(Addrr - i + 1) == 0x8B && *(uint8_t*)(Addrr - i + 2) == 0xC4)
                    return Addrr - i;
        }

    return Memcury::Scanner::FindPattern("40 53 41 56 48 81 EC ? ? ? ? 48 8B 01 48 8B DA 4C 8B F1 FF 90").Get();
}

static inline std::vector<uint64_t> NullFuncs = {};
static inline std::vector<uint64_t> RetTrueFuncs = {};

inline void FindNullsAndRetTrues()
{
    if (VersionInfo.EngineVersion == 4.16)
    {
        NullFuncs.push_back(Memcury::Scanner::FindPattern("4C 89 44 24 ? 88 54 24 ? 48 89 4C 24 ? 56 57 48 81 EC ? ? ? ? 33 C0 83 F8 ? 0F 84 ? ? ? ? B8").Get());
        NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 54 24 ? 48 89 4C 24 ? 55 53 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 8B 41 08 C1 E8 05").Get());
        NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 54 24 ? 48 89 4C 24 ? 55 53 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 8B 41 ? C1 E8 ? A8 ? 0F 84 ? ? ? ? 80 3D").Get());
    }
    else if (VersionInfo.FortniteVersion > 3.2 && VersionInfo.EngineVersion == 4.20) 
    {
        if (VersionInfo.FortniteVersion == 4.1) 
            NullFuncs.push_back(Memcury::Scanner::FindPattern("4C 8B DC 55 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 49 89 5B 10 48 8D 05 ? ? ? ? 48 8B 1D ? ? ? ? 49 89 73 18 33 F6 40").Get());

        NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 57 48 81 EC ? ? ? ? 4C 8B 82 ? ? ? ? 48 8B F9 0F 29 70 E8 0F 29 78 D8").Get());
    }
    else if (VersionInfo.EngineVersion == 4.21) 
    {
        NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 70 10 57 48 81 EC ? ? ? ? 48 8B BA ? ? ? ? 48 8B DA 0F 29").Get());
        NullFuncs.push_back(Memcury::Scanner::FindStringRef(L"Widget Class %s - Running Initialize On Archetype, %s.").ScanFor(VersionInfo.FortniteVersion < 6.3 ? std::vector<uint8_t>{ 0x40, 0x55 } : std::vector<uint8_t>{ 0x48, 0x89, 0x5C }, false).Get());
        NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 41 0F B6 F0 48 8D 15 ? ? ? ? 48 8B F9 41 B8").Get());
    }
    else if (VersionInfo.EngineVersion >= 4.22 && VersionInfo.FortniteVersion <= 12.00) 
        NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 30 48 8B 41 28 48 8B DA 48 8B F9 48 85 C0 74 34 48 8B 4B 08 48 8D").Get());
    else if (VersionInfo.EngineVersion == 4.25)
    {
        NullFuncs.push_back(Memcury::Scanner::FindStringRef(L"Widget Class %s - Running Initialize On Archetype, %s.").ScanFor(std::vector<uint8_t>{ 0x48, 0x89, 0x5C }, false).Get()); // for 12.41
        NullFuncs.push_back(Memcury::Scanner::FindPattern(VersionInfo.FortniteVersion == 12.41 ? "40 57 41 56 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F B6 FA 44 8B F1 74 3A 80 3D ? ? ? ? ? 0F" : "40 57 41 56 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F B6 FA 44 8B F1 74 3A 80 3D ? ? ? ? ? 0F 82").Get());
        if (VersionInfo.FortniteVersion == 12.41)
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 54 41 56 41 57 48 83 EC 70 48 8B 35").Get());
        else if (VersionInfo.FortniteVersion == 12.61)
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 20 4C 8B A5").Get());
    }
    else if (VersionInfo.FortniteVersion == 14.60)
        NullFuncs.push_back(Memcury::Scanner::FindPattern("40 55 57 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F B6 FA 44 8B F9 74 3B 80 3D ? ? ? ? ? 0F").Get());
    else if (VersionInfo.FortniteVersion >= 17.00) {
        //NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 10 48 89 6C 24 20 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B F9").Get());
        if (std::floor(VersionInfo.FortniteVersion) == 17) 
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 70 08 48 89 78 10 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC ? ? ? ? 45 33 ED").Get());
        else if (VersionInfo.FortniteVersion >= 19.00 && VersionInfo.FortniteVersion < 20.00)
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 55 41 56 48 8B EC 48 83 EC 50 83 65 28 00 40 B6 05 40 38 35 ? ? ? ? 4C").Get());
        else if (VersionInfo.FortniteVersion >= 20.00) {
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 4C 89 40 18 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 98 48 81 EC ? ? ? ? 49 8B 48 20 45 33").Get());
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 20 48 8B 41 20 48 8B FA 48 8B D9 BA ? ? ? ? 83 78 08 03 0F 8D").Get());
            NullFuncs.push_back(Memcury::Scanner::FindPattern("4C 89 44 24 ? 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 68 48 8D 05 ? ? ? ? 0F").Get());
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 48 8B F9 48 8B CA E8").Get());
            NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 ? 41 ? 48 83 EC 60 45 33 F6 4C 8D ? ? ? ? ? 48 8B DA").Get());
        }
    }

    if (VersionInfo.FortniteVersion == 2.5) 
        NullFuncs.push_back(Memcury::Scanner::FindPattern("40 55 56 41 56 48 8B EC 48 81 EC ? ? ? ? 48 8B 01 4C 8B F2").Get());
    else 
    {
        auto sRef = Memcury::Scanner::FindStringRef(L"Changing GameSessionId from '%s' to '%s'");
        NullFuncs.push_back(sRef.ScanFor(VersionInfo.EngineVersion >= 4.27 ? std::vector<uint8>{ 0x48, 0x89, 0x5C } : std::vector<uint8>{ 0x40, 0x55 }, false, 0, 1, 2000).Get());
    }

    if (VersionInfo.EngineVersion < 4.27) {
        auto sRef = Memcury::Scanner::FindStringRef(L"STAT_CollectGarbageInternal").Get();
        uint64_t CollectGarbage = 0;

        for (int i = 0; i < 1000; i++) {
            auto Ptr = (uint8_t*)(sRef - i);

            if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C) {
                CollectGarbage = uint64_t(Ptr);
                break;
            }
            else if (*Ptr == 0x40 && *(Ptr + 1) == 0x55) {
                CollectGarbage = uint64_t(Ptr);
                break;
            }
            else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4) {
                CollectGarbage = uint64_t(Ptr);
                break;
            }
        }
        NullFuncs.push_back(CollectGarbage);
    }

    NullFuncs.push_back(FindKickPlayer());
    if (VersionInfo.FortniteVersion == 1.10 || VersionInfo.FortniteVersion == 1.11 || (VersionInfo.FortniteVersion >= 2.2 && VersionInfo.FortniteVersion <= 2.4)) 
        RetTrueFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 57 41 56 41 57 48 81 EC ? ? ? ? 48 8B 01 49 8B E9 45 0F B6 F8").Get());
    else if (VersionInfo.EngineVersion >= 4.26) {
        if (std::floor(VersionInfo.FortniteVersion) == 17) 
            RetTrueFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8B EC 48 83 EC 60 4D 8B F9 41 8A F0 4C 8B F2 48 8B F9 45 32 E4").Get());
        RetTrueFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8B EC 48 83 EC 60 49 8B D9 45 8A").Get());
    }

    if (VersionInfo.FortniteVersion >= 16.00)
    {
        auto RequestExit = Memcury::Scanner::FindPattern("40 53 48 83 EC ? 80 3D ? ? ? ? ? 0F B6 D9 72 ? 48 8D 05 ? ? ? ? 89 5C 24 ? 41 B9 ? ? ? ? 48 89 44 24 ? 4C 8D 05 ? ? ? ? 33 D2 33 C9 E8 ? ? ? ? 48 8D 0D").Get();
        if (!RequestExit)
            RequestExit = Memcury::Scanner::FindPattern("88 4C 24 ? 53 48 83 EC ? 80 3D ? ? ? ? ? 8A D9").Get();

        if (RequestExit)
            NullFuncs.push_back(RequestExit);
    }

    if (VersionInfo.FortniteVersion >= 19.00 && VersionInfo.FortniteVersion <= 24.00)
    {
        auto NoFortLocalPlayer = Memcury::Scanner::FindStringRef(L"%s - No FortLocalPlayer").ScanFor({ 0x48, 0x83, 0xEC }, false).Get();

        if (NoFortLocalPlayer)
            NullFuncs.push_back(NoFortLocalPlayer);


        auto NoLocalPlayer = Memcury::Scanner::FindStringRef(L"%s - No LocalPlayer").ScanFor({ 0x48, 0x83, 0xEC }, false).Get();

        if (NoLocalPlayer)
            NullFuncs.push_back(NoLocalPlayer);
    }


    auto PedestalBeginPlay = Memcury::Scanner::FindStringRef(L"AFortTeamMemberPedestal::BeginPlay - Begun play on pedestal %s").Get();

    if (PedestalBeginPlay)
        for (int i = 0; i < 1000; i++) 
        {
            auto Ptr = (uint8_t*)(PedestalBeginPlay - i);

            if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c) {
                NullFuncs.push_back((uint64_t)Ptr);
                break;
            }
            else if (*Ptr == 0x40 && *(Ptr + 1) == 0x53 && *(Ptr + 2) == 0x41 && *(Ptr + 3) == 0x56) {
                NullFuncs.push_back((uint64_t)Ptr);
                break;
            }
        }

    if (VersionInfo.EngineVersion >= 4.21)
    {
        if (VersionInfo.EngineVersion == 4.21 || VersionInfo.EngineVersion == 4.22)
            RetTrueFuncs.push_back(Memcury::Scanner::FindPattern("4C 89 4C 24 20 55 56 57 41 56 48 8D 6C 24 D1").Get());
        else
        {
            auto sRef = Memcury::Scanner::FindStringRef(L"CanActivateAbility %s failed, blueprint refused", true, 0, VersionInfo.EngineVersion >= 5.0).Get();

            for (int i = 0; i < 1000; i++) {
                auto Ptr = (uint8_t*)(sRef - i);

                if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5C) {
                    RetTrueFuncs.push_back(uint64_t(Ptr));
                    break;
                }
                else if (*Ptr == 0x48 && *(Ptr + 1) == 0x8B && *(Ptr + 2) == 0xC4) {
                    RetTrueFuncs.push_back(uint64_t(Ptr));
                    break;
                }
            }
        }
    }
}