#pragma once

namespace Hooking
{
    inline void* __nullptrForHook = nullptr;

    bool InternalHook(uintptr_t Ptr, void* Detour, void** Original);

    template <class PtrType = void*, class OriginalType = void*>
    std::enable_if_t<!std::is_base_of_v<UObject, OriginalType>, void> Hook(PtrType _Ptr, void* _Detour, OriginalType& _Orig = __nullptrForHook)
    {
        InternalHook((uintptr_t)_Ptr, _Detour, (LPVOID*)(std::is_same_v<OriginalType, void*> ? nullptr : &_Orig));
    }

    bool EnableHook(uintptr_t Ptr);
    bool DisableHook(uintptr_t Ptr);

    inline void VirtualSwap(void** VTable, int Index, void* Detour, void** Original = nullptr);

    template <typename ClassType, typename OriginalType = void*>
    __forceinline void Hook(uint32_t _Ind, void* _Detour, OriginalType& _Orig = __nullptrForHook)
    {
        auto _Vt = ClassType::GetDefaultObj()->Vft;

        if (!std::is_same_v<OriginalType, void*>)
            _Orig = (OriginalType)_Vt[_Ind];

        DWORD _Vo;
        VirtualProtect(_Vt + _Ind, 8, PAGE_EXECUTE_READWRITE, &_Vo);
        _Vt[_Ind] = _Detour;
        VirtualProtect(_Vt + _Ind, 8, _Vo, &_Vo);
    }

    template <typename ClassType>
    __forceinline static void HookEvery(uint32_t _Ind, void* _Detour)
    {
        for (int32_t i = 0; i < TUObjectArray::Num(); i++)
        {
            auto Obj = TUObjectArray::GetObjectByIndex(i);

            if (Obj && Obj->IsDefaultObject() && Obj->IsA<ClassType>())
            {
                auto ObjectVTable = Obj->Vft;

                DWORD og;
                VirtualProtect(ObjectVTable + _Ind, 8, PAGE_EXECUTE_READWRITE, &og);
                ObjectVTable[_Ind] = _Detour;
                VirtualProtect(ObjectVTable + _Ind, 8, og, &og);
            }
        }
    }

    template <typename OriginalType = void*>
    __forceinline void ExecHook(UFunction* _Fn, void* _Detour, OriginalType& _Orig = __nullptrForHook)
    {
        if (!_Fn)
            return;
        if (!std::is_same_v<OriginalType, void*>)
            _Orig = (OriginalType)_Fn->ExecFunction;

        _Fn->ExecFunction = (void (*)(void*, void*, void*))_Detour;
    }

    template <typename PatchType>
    __forceinline void Patch(uintptr_t ptr, PatchType byte)
    {
        DWORD og;
        VirtualProtect(LPVOID(ptr), sizeof(PatchType), PAGE_EXECUTE_READWRITE, &og);
        *(PatchType*)ptr = byte;
        VirtualProtect(LPVOID(ptr), sizeof(PatchType), og, &og);
    }

    void Null(uintptr_t Ptr);
    void RetTrue(uintptr_t Ptr);

    uint8_t* AllocateShellcodeRegion(void* targetAddr, size_t SizeOfShellcode);

    void InternalRel32Hook(uintptr_t _Ptr, void* _Detour, void** _Orig);

    template <class PtrType = void*, class OriginalType = void*>
    void Rel32Hook(PtrType _Ptr, void* _Detour, OriginalType& _Orig = __nullptrForHook)
    {
        InternalRel32Hook((uintptr_t)_Ptr, _Detour, (LPVOID*)(std::is_same_v<OriginalType, void*> ? nullptr : &_Orig));
    }
} // namespace Hooking