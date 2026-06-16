#include "pch.h"
#include "../../../Erbium/Erbium/Public/hde64.hpp"

struct FHookData
{
    uint8_t OriginalBytes[5];
    uint8_t HookBytes[5];
};

std::unordered_map<uintptr_t, FHookData> InternalHooks;

__forceinline void SetupJmp(uintptr_t Ptr, uint8_t* ShellcodeAddr)
{
    FHookData HookData;
    memcpy(HookData.OriginalBytes, LPVOID(Ptr), 5);

    auto Rel32 = uintptr_t(ShellcodeAddr) - (Ptr + 5);

    DWORD og;
    VirtualProtect(LPVOID(Ptr), 5, PAGE_EXECUTE_READWRITE, &og);

    *(uint8_t*)Ptr = 0xE9;
    *(uint32_t*)(Ptr + 1) = uint32_t(Rel32);

    VirtualProtect(LPVOID(Ptr), 5, og, &og);

    memcpy(HookData.HookBytes, LPVOID(Ptr), 5);

    InternalHooks[Ptr] = HookData;
}

struct FInstructionData
{
    bool bSucceeded;
    std::vector<uint8_t> OriginalBytes;

    struct RipFixup
    {
        uint32_t offset;
        uintptr_t target;
    };
    std::vector<RipFixup> RipFixups;
};

__forceinline FInstructionData FetchInstructionData(uintptr_t Stream)
{
    auto Length = 0;
    auto jmpDest = (uintptr_t)0;
    FInstructionData Result;

    while (Length < 5 || (uintptr_t)(Stream + Length) <= jmpDest)
    {
        hde64s hs;
        const uintptr_t pOldInst = Stream + Length;

        auto InsnSize = hde64_disasm((LPVOID)pOldInst, &hs);

        if (hs.flags & F_ERROR)
            return { false };

        if ((hs.modrm & 0xC7) == 0x05)
        {
            const uintptr_t absTarget = pOldInst + hs.len + (int32_t)hs.disp.disp32;

            const uint32_t insnOffset = (uint32_t)Result.OriginalBytes.size();
            Result.OriginalBytes.insert(Result.OriginalBytes.end(), (uint8_t*)pOldInst, (uint8_t*)pOldInst + hs.len);

            const uint32_t dispOffset = insnOffset + hs.len - ((hs.flags & 0x3C) >> 2) - 4;

            Result.RipFixups.push_back({ dispOffset, absTarget });

            if (hs.opcode == 0xFF && hs.modrm_reg == 4)
            {
                Length += hs.len;
                goto done;
            }
        }
        else if (hs.opcode == 0xE8)
        {
            const uintptr_t dest = pOldInst + hs.len + (int32_t)hs.imm.imm32;

            uint8_t stub[] = { 0xFF, 0x15, 0x02, 0x00, 0x00, 0x00, 0xEB, 0x08, 0, 0, 0, 0, 0, 0, 0, 0 };
            memcpy(stub + 8, &dest, sizeof(dest));
            Result.OriginalBytes.insert(Result.OriginalBytes.end(), stub, stub + sizeof(stub));
        }
        else if ((hs.opcode & 0xFD) == 0xE9)
        {
            uintptr_t dest = pOldInst + hs.len;
            if (hs.opcode == 0xEB)
                dest += (int8_t)hs.imm.imm8;
            else
                dest += (int32_t)hs.imm.imm32;

            if (Stream <= dest && dest < Stream + 5)
            {
                if (jmpDest < dest)
                    jmpDest = dest;
                Result.OriginalBytes.insert(Result.OriginalBytes.end(), (uint8_t*)pOldInst, (uint8_t*)pOldInst + hs.len);
            }
            else
            {
                uint8_t stub[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 };
                memcpy(stub + 6, &dest, sizeof(dest));
                Result.OriginalBytes.insert(Result.OriginalBytes.end(), stub, stub + sizeof(stub));

                if (pOldInst >= jmpDest)
                {
                    Length += hs.len;
                    goto done;
                }
            }
        }
        else if ((hs.opcode & 0xF0) == 0x70 || (hs.opcode & 0xFC) == 0xE0 || (hs.opcode2 & 0xF0) == 0x80)
        {
            if ((hs.opcode & 0xFC) == 0xE0)
                return { false };

            uintptr_t dest = pOldInst + hs.len;
            if ((hs.opcode & 0xF0) == 0x70)
                dest += (int8_t)hs.imm.imm8;
            else
                dest += (int32_t)hs.imm.imm32;

            if (Stream <= dest && dest < Stream + 5)
            {
                if (jmpDest < dest)
                    jmpDest = dest;
                Result.OriginalBytes.insert(Result.OriginalBytes.end(), (uint8_t*)pOldInst, (uint8_t*)pOldInst + hs.len);
            }
            else
            {
                const uint8_t cond = ((hs.opcode != 0x0F ? hs.opcode : hs.opcode2) & 0x0F);

                uint8_t stub[] = { (uint8_t)(0x71 ^ cond), 0x0E, 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 };
                memcpy(stub + 8, &dest, sizeof(dest));
                Result.OriginalBytes.insert(Result.OriginalBytes.end(), stub, stub + sizeof(stub));
            }
        }
        else
            Result.OriginalBytes.insert(Result.OriginalBytes.end(), (uint8_t*)pOldInst, (uint8_t*)pOldInst + hs.len);

        Length += hs.len;

        if ((hs.opcode & 0xFE) == 0xC2)
            break;
    }

done:
    if (Length < 5)
        return { false };

    const uintptr_t resumeAddr = Stream + Length;
    uint8_t tailJmp[] = { 0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0, 0, 0, 0, 0, 0, 0, 0 };
    memcpy(tailJmp + 6, &resumeAddr, sizeof(resumeAddr));
    Result.OriginalBytes.insert(Result.OriginalBytes.end(), tailJmp, tailJmp + sizeof(tailJmp));

    Result.bSucceeded = true;
    return Result;
}

bool Hooking::InternalHook(uintptr_t Ptr, void* Detour, void** Original)
{
    if (!Ptr)
        return false;

    // clang-format off
    uint8_t Shellcode[] =
	{
        0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, // jmp qword ptr [rip]
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
    // clang-format on

    bool bJmp = (*(uint8_t*)Ptr & 0xFE) == 0xE8;
    auto InsnData = FetchInstructionData(Ptr);

    if (!InsnData.bSucceeded)
        return false;

    auto ShellcodeAddr = AllocateShellcodeRegion((void*)Ptr, sizeof(Shellcode) + ((bJmp || !Original) ? 0 : InsnData.OriginalBytes.size()));

    if (!ShellcodeAddr)
        return false;

    *(uint64_t*)(Shellcode + 6) = uint64_t(Detour);
    memcpy(ShellcodeAddr, Shellcode, sizeof(Shellcode));

    if (Original)
    {
        if (bJmp)
        {
            auto Orig = (Ptr + 5) + *(int32_t*)(Ptr + 1);

            *Original = (void*)Orig;
        }
        else
        {
            uint8_t* const TrampolineBase = ShellcodeAddr + sizeof(Shellcode);

            memcpy(TrampolineBase, InsnData.OriginalBytes.data(), InsnData.OriginalBytes.size());

            for (auto& Fixup : InsnData.RipFixups)
            {
                uint8_t* const DispPtr = TrampolineBase + Fixup.offset;

                const int32_t NewDisp = (int32_t)(Fixup.target - ((uintptr_t)DispPtr + 4));

                memcpy(DispPtr, &NewDisp, sizeof(NewDisp));
            }

            *Original = TrampolineBase;
        }
    }

    SetupJmp(Ptr, ShellcodeAddr);

    return true;
}

bool Hooking::DisableHook(uint64_t Ptr)
{
    if (InternalHooks.contains(Ptr))
    {
        DWORD og;
        VirtualProtect(LPVOID(Ptr), 5, PAGE_EXECUTE_READWRITE, &og);

        memcpy(LPVOID(Ptr), InternalHooks[Ptr].OriginalBytes, 5);

        VirtualProtect(LPVOID(Ptr), 5, og, &og);

        return true;
    }

    return false;
}

bool Hooking::EnableHook(uint64_t Ptr)
{
    if (InternalHooks.contains(Ptr))
    {
        DWORD og;
        VirtualProtect(LPVOID(Ptr), 5, PAGE_EXECUTE_READWRITE, &og);

        memcpy(LPVOID(Ptr), InternalHooks[Ptr].HookBytes, 5);

        VirtualProtect(LPVOID(Ptr), 5, og, &og);

        return true;
    }

    return false;
}

void Hooking::VirtualSwap(void** VTable, int Index, void* Detour, void** Original)
{
    if (Original)
        *Original = VTable[Index];

    DWORD dwProt;
    VirtualProtect(&VTable[Index], sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProt);

    VTable[Index] = Detour;

    DWORD dwTemp;
    VirtualProtect(&VTable[Index], sizeof(void*), dwProt, &dwTemp);
}

void Hooking::Null(uintptr_t Ptr)
{
    Hooking::Patch<uint8_t>(Ptr, 0xC3);
}

void Hooking::RetTrue(uintptr_t Ptr)
{
    Hooking::Patch<uint32_t>(Ptr, 0xC0FFC031);
    Hooking::Patch<uint8_t>(Ptr + 4, 0xC3);
}

struct FBlock
{
    void* Page;
    size_t CurrentPtr;
};

std::vector<FBlock> Blocks;

uint8_t* Hooking::AllocateShellcodeRegion(void* targetAddr, size_t SizeOfShellcode)
{
    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);

    const uint64_t PageSize = SysInfo.dwPageSize;
    const uint64_t StartAddr = (uint64_t(targetAddr) & ~(PageSize - 1));
    const uint64_t MinAddr = max(StartAddr - 0x7FFFFF00, (uint64_t)SysInfo.lpMinimumApplicationAddress);
    const uint64_t MaxAddr = min(StartAddr + 0x7FFFFF00, (uint64_t)SysInfo.lpMaximumApplicationAddress);
    const uint64_t StartPage = (StartAddr - (StartAddr % PageSize));

    for (auto& Block : Blocks)
    {
        if (uint64_t(Block.Page) >= MaxAddr || uint64_t(Block.Page) < MinAddr)
            continue;

        auto Ptr = (uint8_t*)Block.Page + Block.CurrentPtr;

        Block.CurrentPtr += SizeOfShellcode;

        return Ptr;
    }

    for (uint64_t PageOffset = 1; PageOffset; PageOffset++)
    {
        uint64_t ByteOffset = PageOffset * PageSize;
        uint64_t HighAddr = StartPage + ByteOffset;
        uint64_t LowAddr = (StartPage > ByteOffset) ? StartPage - ByteOffset : 0;

        bool NeedsExit = HighAddr > MaxAddr && LowAddr < MinAddr;

        if (HighAddr < MaxAddr)
        {
            if (void* OutAddr = VirtualAlloc((void*)HighAddr, PageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))
            {
                Blocks.push_back({ OutAddr, SizeOfShellcode });
                return (uint8_t*)OutAddr;
            }
        }

        if (LowAddr > MinAddr)
        {
            if (void* OutAddr = VirtualAlloc((void*)LowAddr, PageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))
            {
                Blocks.push_back({ OutAddr, SizeOfShellcode });
                return (uint8_t*)OutAddr;
            }
        }

        if (NeedsExit)
        {
            break;
        }
    }

    return nullptr;
}

void Hooking::InternalRel32Hook(uintptr_t Ptr, void* Detour, void** Original)
{
    auto Orig = (Ptr + 4) + *(int32_t*)Ptr;

    if (Original != nullptr)
        *Original = (void*)Orig;

    // clang-format off
    uint8_t Shellcode[] =
	{
        0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, // jmp qword ptr [rip]
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
    // clang-format on

    auto NearbyPage = AllocateShellcodeRegion((void*)Ptr, sizeof(Shellcode));

    *(uint64_t*)(Shellcode + 6) = uint64_t(Detour);
    memcpy(NearbyPage, Shellcode, sizeof(Shellcode));

    auto NewRel32 = uint64_t(NearbyPage) - (Ptr + 4);
    Hooking::Patch<uint32_t>(Ptr, (uint32_t)NewRel32);
}