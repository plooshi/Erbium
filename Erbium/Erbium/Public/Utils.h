#pragma once
#include "../../pch.h"
#include "MinHook.h"
#include "Finders.h"


class Utils {
    static inline void* _NpFH = nullptr;
public:
    template <class _Ot = void*>
    static void Hook(uint64_t _Ptr, void* _Detour, _Ot& _Orig = _NpFH)
    {
        MH_CreateHook((LPVOID)_Ptr, _Detour, (LPVOID*)(std::is_same_v<_Ot, void*> ? nullptr : &_Orig));
    }

    __forceinline static void _HookVT(void** _Vt, uint32_t _Ind, void* _Detour)
    {
        DWORD _Vo;
        VirtualProtect(_Vt + _Ind, 8, PAGE_EXECUTE_READWRITE, &_Vo);
        _Vt[_Ind] = _Detour;
        VirtualProtect(_Vt + _Ind, 8, _Vo, &_Vo);
    }

    template <typename _Ct, typename _Ot = void*>
    __forceinline static void Hook(uint32_t _Ind, void* _Detour, _Ot& _Orig = _NpFH)
    {
        auto _Vt = _Ct::GetDefaultObj()->Vft;
        if (!std::is_same_v<_Ot, void*>)
            _Orig = (_Ot)_Vt[_Ind];

        _HookVT(_Vt, _Ind, _Detour);
    }

    template <typename _Ct>
    __forceinline static void HookEvery(uint32_t _Ind, void* _Detour)
    {
        for (int i = 0; i < TUObjectArray::Num(); i++)
        {
            auto Obj = TUObjectArray::GetObjectByIndex(i);
            if (Obj && Obj->IsDefaultObject() && Obj->IsA<_Ct>())
            {
                _HookVT(Obj->Vft, _Ind, _Detour);
            }
        }
    }

    template <typename _Ct, typename _Ot = void*>
    __forceinline static void ExecHookEvery(const char* ShortName, void* _Detour, _Ot& _Orig = _NpFH)
    {
        for (int i = 0; i < TUObjectArray::Num(); i++)
        {
            auto Obj = TUObjectArray::GetObjectByIndex(i);
            if (Obj && Obj->IsA<_Ct>())
            {
                ExecHook(Obj->Class->GetFunction(ShortName)->GetFullName().c_str(), _Detour, _Orig);
            }
        }
    }

    template <typename _Is>
    static __forceinline void Patch(uintptr_t ptr, _Is byte)
    {
        DWORD og;
        VirtualProtect(LPVOID(ptr), sizeof(_Is), PAGE_EXECUTE_READWRITE, &og);
        *(_Is*)ptr = byte;
        VirtualProtect(LPVOID(ptr), sizeof(_Is), og, &og);
    }

    static void GetAllInternal(const UClass* Class, TArray<AActor*>& ret)
    {
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), Class, &ret);
    }

    template <typename _At = AActor>
    __forceinline static void GetAll(const UClass* Class, TArray<_At*>& ret)
    {
        return GetAllInternal(Class, *(TArray<AActor*>*)&ret);
    }

    template <typename _At = AActor>
    __forceinline static void GetAll(TArray<_At*>& ret)
    {
        GetAllInternal(_At::StaticClass(), *(TArray<AActor*>*)&ret);
    }

    template <typename _Ot = void*>
    __forceinline static void ExecHook(UFunction* _Fn, void* _Detour, _Ot& _Orig = _NpFH)
    {
        if (!_Fn)
            return;
        if (!std::is_same_v<_Ot, void*>)
            _Orig = (_Ot)_Fn->ExecFunction;

        _Fn->ExecFunction = _Detour;
    }


    template <typename _Ot = void*>
    __forceinline static void ExecHook(const wchar_t* _Name, void* _Detour, _Ot& _Orig = _NpFH)
    {
        UFunction* _Fn = (UFunction*)FindObject<UFunction>(_Name);
        ExecHook(_Fn, _Detour, _Orig);
    }

    static double precision(double f, double places)
    {
        double n = pow(10., places);
        return round(f * n) / n;
    }
};

#define DEFINE_NEWOBJ_PROP(Name, ...)                                                                                                                                \
    static inline int32 Name##__Offset = -2;                                                                                                                         \
    static inline bool Name##__Weak = false;                                                                                                                         \
    __VA_ARGS__* Get##Name() const                                                                                                                                   \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = this->GetProperty(#Name, 0x8010000);                                                                                                         \
            if (!Prop)                                                                                                                                               \
                Name##__Offset = -1;                                                                                                                                 \
            else if (VersionInfo.EngineVersion >= 5.1)                                                                                                               \
            {                                                                                                                                                        \
		        auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                    \
                auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                          \
                Name##__Weak = (FieldFlags & 0x8000000) != 0;                                                                                                        \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
            }                                                                                                                                                        \
            else                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
        }                                                                                                                                                            \
        return Name##__Weak ? GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset).Get() : GetFromOffset<__VA_ARGS__*>(this, Name##__Offset);            \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    bool Has##Name() const                                                                                                                                           \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
            Name##__Offset = this->GetOffset(#Name, GUESS_PROP_FLAGS(__VA_ARGS__));                                                                                  \
        return Name##__Offset != -1;                                                                                                                                 \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __VA_ARGS__* Set##Name(__VA_ARGS__*&& Value) const                                                                                                               \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = this->GetProperty(#Name, 0x8010000);                                                                                                         \
            if (!Prop)                                                                                                                                               \
                Name##__Offset = -1;                                                                                                                                 \
            else if (VersionInfo.EngineVersion >= 5.1)                                                                                                               \
            {                                                                                                                                                        \
		        auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                    \
                auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                          \
                Name##__Weak = (FieldFlags & 0x8000000) != 0;                                                                                                        \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
            }                                                                                                                                                        \
            else                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
        }                                                                                                                                                            \
        if (Name##__Weak)                                                                                                                                            \
            GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset) = Value;                                                                                \
        else                                                                                                                                                         \
            GetFromOffset<__VA_ARGS__*>(this, Name##__Offset) = Value;                                                                                               \
        return Value;                                                                                                                                                \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __VA_ARGS__* Set##Name(__VA_ARGS__*& Value) const                                                                                                                \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = this->GetProperty(#Name, 0x8010000);                                                                                                         \
            if (!Prop)                                                                                                                                               \
                Name##__Offset = -1;                                                                                                                                 \
            else if (VersionInfo.EngineVersion >= 5.1)                                                                                                               \
            {                                                                                                                                                        \
		        auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                    \
                auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                          \
                Name##__Weak = (FieldFlags & 0x8000000) != 0;                                                                                                        \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
            }                                                                                                                                                        \
            else                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
        }                                                                                                                                                            \
        if (Name##__Weak)                                                                                                                                            \
            GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset) = Value;                                                                                \
        else                                                                                                                                                         \
            GetFromOffset<__VA_ARGS__*>(this, Name##__Offset) = Value;                                                                                               \
        return Value;                                                                                                                                                \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __declspec(property(get = Get##Name, put = Set##Name))                                                                                                           \
    __VA_ARGS__* Name;


#define DEFINE_STRUCT_NEWOBJ_PROP(Name, ...)                                                                                                                         \
    static inline int32 Name##__Offset = -2;                                                                                                                         \
    static inline bool Name##__Weak = false;                                                                                                                         \
    __VA_ARGS__* Get##Name() const                                                                                                                                   \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x8010000);                                                                                               \
            if (!Prop)                                                                                                                                               \
                Name##__Offset = -1;                                                                                                                                 \
            else if (VersionInfo.EngineVersion >= 5.1)                                                                                                               \
            {                                                                                                                                                        \
		        auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                    \
                auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                          \
                Name##__Weak = (FieldFlags & 0x8000000) != 0;                                                                                                        \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
            }                                                                                                                                                        \
            else                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
        }                                                                                                                                                            \
        return Name##__Weak ? GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset).Get() : GetFromOffset<__VA_ARGS__*>(this, Name##__Offset);            \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    bool Has##Name() const                                                                                                                                           \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x8010000);                                                                                               \
            if (!Prop)                                                                                                                                               \
                Name##__Offset = -1;                                                                                                                                 \
            else if (VersionInfo.EngineVersion >= 5.1)                                                                                                               \
            {                                                                                                                                                        \
		        auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                    \
                auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                          \
                Name##__Weak = (FieldFlags & 0x8000000) != 0;                                                                                                        \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
            }                                                                                                                                                        \
            else                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
        }                                                                                                                                                            \
        return Name##__Offset != -1;                                                                                                                                 \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __VA_ARGS__* Set##Name(__VA_ARGS__*&& Value) const                                                                                                               \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x8010000);                                                                                               \
            if (!Prop)                                                                                                                                               \
                Name##__Offset = -1;                                                                                                                                 \
            else if (VersionInfo.EngineVersion >= 5.1)                                                                                                               \
            {                                                                                                                                                        \
		        auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                    \
                auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                          \
                Name##__Weak = (FieldFlags & 0x8000000) != 0;                                                                                                        \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
            }                                                                                                                                                        \
            else                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
        }                                                                                                                                                            \
        if (Name##__Weak)                                                                                                                                            \
            GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset) = Value;                                                                                \
        else                                                                                                                                                         \
            GetFromOffset<__VA_ARGS__*>(this, Name##__Offset) = Value;                                                                                               \
        return Value;                                                                                                                                                \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __VA_ARGS__* Set##Name(__VA_ARGS__*& Value) const                                                                                                                \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x8010000);                                                                                               \
            if (!Prop)                                                                                                                                               \
                Name##__Offset = -1;                                                                                                                                 \
            else if (VersionInfo.EngineVersion >= 5.1)                                                                                                               \
            {                                                                                                                                                        \
		        auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                    \
                auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                          \
                Name##__Weak = (FieldFlags & 0x8000000) != 0;                                                                                                        \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
            }                                                                                                                                                        \
            else                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                              \
        }                                                                                                                                                            \
        if (Name##__Weak)                                                                                                                                            \
            GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset) = Value;                                                                                \
        else                                                                                                                                                         \
            GetFromOffset<__VA_ARGS__*>(this, Name##__Offset) = Value;                                                                                               \
        return Value;                                                                                                                                                \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __declspec(property(get = Get##Name, put = Set##Name))                                                                                                           \
    __VA_ARGS__* Name;

inline std::vector<void(*)()> _HookFuncs;
inline std::vector<void(*)()> _PostLoadHookFuncs;
#define DefHookOg(_Rt, _Name, ...) static inline _Rt (*_Name##OG)(##__VA_ARGS__); static _Rt _Name(##__VA_ARGS__); 
#define DefUHookOg(_Name) static inline void (*_Name##OG)(UObject*, FFrame&); static void _Name(UObject*, FFrame&); 
#define DefUHookOgRet(_Rt, _Name) static inline void (*_Name##OG)(UObject*, FFrame&, _Rt*); static void _Name(UObject *, FFrame&, _Rt*);
#ifdef CLIENT
#define InitHooks static void Hook();
#define InitPostLoadHooks static void PostLoadHook();
#else
#define InitHooks static void Hook(); static int _AddHook() { _HookFuncs.push_back(Hook); return 0; }; static inline auto _HookAdder = _AddHook();
#define InitPostLoadHooks static void PostLoadHook(); static int _AddPostLoadHook() { _PostLoadHookFuncs.push_back(PostLoadHook); return 0; }; static inline auto _PostLoadHookAdder = _AddPostLoadHook();
#endif
#define callOG(_Tr, _Fn, _Th, ...) ([&](){ _Fn->ExecFunction = _Th##_OG; _Tr->_Th(##__VA_ARGS__); _Fn->ExecFunction = _Th##_; })()
#define callOGWithRet(_Tr, _Fn, _Th, ...) ([&](){ _Fn->ExecFunction = _Th##_OG; auto _Rt = _Tr->_Th(##__VA_ARGS__); _Fn->ExecFunction = _Th##_; return _Rt; })()
