#pragma once
#include "../../pch.h"
#include "Finders.h"
#include "Hooking.hpp"

class Utils
{
    static inline void* _NpFH = nullptr;

public:
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

    static double precision(double f, double places)
    {
        double n = pow(10., places);
        return round(f * n) / n;
    }
};

#define DEFINE_NEWOBJ_PROP(Name, ...)                                                                                                                                                                                 \
    static inline int32 Name##__Offset = -2;                                                                                                                                                                          \
    static inline bool Name##__Weak = false;                                                                                                                                                                          \
    __VA_ARGS__* Get##Name() const                                                                                                                                                                                    \
    {                                                                                                                                                                                                                 \
        if (Name##__Offset == -2)                                                                                                                                                                                     \
        {                                                                                                                                                                                                             \
            auto Prop = this->GetProperty(#Name, 0x8010000);                                                                                                                                                          \
            if (!Prop)                                                                                                                                                                                                \
                Name##__Offset = -1;                                                                                                                                                                                  \
            else                                                                                                                                                                                                      \
            {                                                                                                                                                                                                         \
                if (VersionInfo.FortniteVersion >= 12.10)                                                                                                                                                             \
                {                                                                                                                                                                                                     \
                    auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                                                                 \
                    auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                                                                       \
                    if ((FieldFlags & 0x8000000) != 0)                                                                                                                                                                \
                        Name##__Weak = true;                                                                                                                                                                          \
                }                                                                                                                                                                                                     \
                                                                                                                                                                                                                      \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                                                                               \
            }                                                                                                                                                                                                         \
        }                                                                                                                                                                                                             \
        return Name##__Weak ? GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset).Get() : GetFromOffset<__VA_ARGS__*>(this, Name##__Offset);                                                             \
    }                                                                                                                                                                                                                 \
                                                                                                                                                                                                                      \
    bool Has##Name() const                                                                                                                                                                                            \
    {                                                                                                                                                                                                                 \
        if (Name##__Offset == -2)                                                                                                                                                                                     \
        {                                                                                                                                                                                                             \
            auto Prop = this->GetProperty(#Name, 0x8010000);                                                                                                                                                          \
            if (!Prop)                                                                                                                                                                                                \
                Name##__Offset = -1;                                                                                                                                                                                  \
            else                                                                                                                                                                                                      \
            {                                                                                                                                                                                                         \
                if (VersionInfo.FortniteVersion >= 12.10)                                                                                                                                                             \
                {                                                                                                                                                                                                     \
                    auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                                                                 \
                    auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                                                                       \
                    if ((FieldFlags & 0x8000000) != 0)                                                                                                                                                                \
                        Name##__Weak = true;                                                                                                                                                                          \
                }                                                                                                                                                                                                     \
                                                                                                                                                                                                                      \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                                                                               \
            }                                                                                                                                                                                                         \
        }                                                                                                                                                                                                             \
        return Name##__Offset != -1;                                                                                                                                                                                  \
    }                                                                                                                                                                                                                 \
                                                                                                                                                                                                                      \
    __VA_ARGS__* Set##Name(__VA_ARGS__*&& Value) const                                                                                                                                                                \
    {                                                                                                                                                                                                                 \
        if (Name##__Offset == -2)                                                                                                                                                                                     \
        {                                                                                                                                                                                                             \
            auto Prop = this->GetProperty(#Name, 0x8010000);                                                                                                                                                          \
            if (!Prop)                                                                                                                                                                                                \
                Name##__Offset = -1;                                                                                                                                                                                  \
            else                                                                                                                                                                                                      \
            {                                                                                                                                                                                                         \
                if (VersionInfo.FortniteVersion >= 12.10)                                                                                                                                                             \
                {                                                                                                                                                                                                     \
                    auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                                                                 \
                    auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                                                                       \
                    if ((FieldFlags & 0x8000000) != 0)                                                                                                                                                                \
                        Name##__Weak = true;                                                                                                                                                                          \
                }                                                                                                                                                                                                     \
                                                                                                                                                                                                                      \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                                                                               \
            }                                                                                                                                                                                                         \
        }                                                                                                                                                                                                             \
        if (Name##__Weak)                                                                                                                                                                                             \
            GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset) = Value;                                                                                                                                 \
        else                                                                                                                                                                                                          \
            GetFromOffset<__VA_ARGS__*>(this, Name##__Offset) = Value;                                                                                                                                                \
        return Value;                                                                                                                                                                                                 \
    }                                                                                                                                                                                                                 \
                                                                                                                                                                                                                      \
    __VA_ARGS__* Set##Name(__VA_ARGS__*& Value) const                                                                                                                                                                 \
    {                                                                                                                                                                                                                 \
        if (Name##__Offset == -2)                                                                                                                                                                                     \
        {                                                                                                                                                                                                             \
            auto Prop = this->GetProperty(#Name, 0x8010000);                                                                                                                                                          \
            if (!Prop)                                                                                                                                                                                                \
                Name##__Offset = -1;                                                                                                                                                                                  \
            else                                                                                                                                                                                                      \
            {                                                                                                                                                                                                         \
                if (VersionInfo.FortniteVersion >= 12.10)                                                                                                                                                             \
                {                                                                                                                                                                                                     \
                    auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                                                                 \
                    auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                                                                       \
                    if ((FieldFlags & 0x8000000) != 0)                                                                                                                                                                \
                        Name##__Weak = true;                                                                                                                                                                          \
                }                                                                                                                                                                                                     \
                                                                                                                                                                                                                      \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                                                                               \
            }                                                                                                                                                                                                         \
        }                                                                                                                                                                                                             \
        if (Name##__Weak)                                                                                                                                                                                             \
            GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset) = Value;                                                                                                                                 \
        else                                                                                                                                                                                                          \
            GetFromOffset<__VA_ARGS__*>(this, Name##__Offset) = Value;                                                                                                                                                \
        return Value;                                                                                                                                                                                                 \
    }                                                                                                                                                                                                                 \
                                                                                                                                                                                                                      \
    __declspec(property(get = Get##Name, put = Set##Name)) __VA_ARGS__* Name;

#define DEFINE_STRUCT_NEWOBJ_PROP(Name, ...)                                                                                                                                                                          \
    static inline int32 Name##__Offset = -2;                                                                                                                                                                          \
    static inline bool Name##__Weak = false;                                                                                                                                                                          \
    __VA_ARGS__* Get##Name() const                                                                                                                                                                                    \
    {                                                                                                                                                                                                                 \
        if (Name##__Offset == -2)                                                                                                                                                                                     \
        {                                                                                                                                                                                                             \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x8010000);                                                                                                                                                \
            if (!Prop)                                                                                                                                                                                                \
                Name##__Offset = -1;                                                                                                                                                                                  \
            else                                                                                                                                                                                                      \
            {                                                                                                                                                                                                         \
                if (VersionInfo.FortniteVersion >= 12.10)                                                                                                                                                             \
                {                                                                                                                                                                                                     \
                    auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                                                                 \
                    auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                                                                       \
                    if ((FieldFlags & 0x8000000) != 0)                                                                                                                                                                \
                        Name##__Weak = true;                                                                                                                                                                          \
                }                                                                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                                                                               \
            }                                                                                                                                                                                                         \
        }                                                                                                                                                                                                             \
        return Name##__Weak ? GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset).Get() : GetFromOffset<__VA_ARGS__*>(this, Name##__Offset);                                                             \
    }                                                                                                                                                                                                                 \
                                                                                                                                                                                                                      \
    bool Has##Name() const                                                                                                                                                                                            \
    {                                                                                                                                                                                                                 \
        if (Name##__Offset == -2)                                                                                                                                                                                     \
        {                                                                                                                                                                                                             \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x8010000);                                                                                                                                                \
            if (!Prop)                                                                                                                                                                                                \
                Name##__Offset = -1;                                                                                                                                                                                  \
            else                                                                                                                                                                                                      \
            {                                                                                                                                                                                                         \
                if (VersionInfo.FortniteVersion >= 12.10)                                                                                                                                                             \
                {                                                                                                                                                                                                     \
                    auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                                                                 \
                    auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                                                                       \
                    if ((FieldFlags & 0x8000000) != 0)                                                                                                                                                                \
                        Name##__Weak = true;                                                                                                                                                                          \
                }                                                                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                                                                               \
            }                                                                                                                                                                                                         \
        }                                                                                                                                                                                                             \
        return Name##__Offset != -1;                                                                                                                                                                                  \
    }                                                                                                                                                                                                                 \
                                                                                                                                                                                                                      \
    __VA_ARGS__* Set##Name(__VA_ARGS__*&& Value) const                                                                                                                                                                \
    {                                                                                                                                                                                                                 \
        if (Name##__Offset == -2)                                                                                                                                                                                     \
        {                                                                                                                                                                                                             \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x8010000);                                                                                                                                                \
            if (!Prop)                                                                                                                                                                                                \
                Name##__Offset = -1;                                                                                                                                                                                  \
            else                                                                                                                                                                                                      \
            {                                                                                                                                                                                                         \
                if (VersionInfo.FortniteVersion >= 12.10)                                                                                                                                                             \
                {                                                                                                                                                                                                     \
                    auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                                                                 \
                    auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                                                                       \
                    if ((FieldFlags & 0x8000000) != 0)                                                                                                                                                                \
                        Name##__Weak = true;                                                                                                                                                                          \
                }                                                                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                                                                               \
            }                                                                                                                                                                                                         \
        }                                                                                                                                                                                                             \
        if (Name##__Weak)                                                                                                                                                                                             \
            GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset) = Value;                                                                                                                                 \
        else                                                                                                                                                                                                          \
            GetFromOffset<__VA_ARGS__*>(this, Name##__Offset) = Value;                                                                                                                                                \
        return Value;                                                                                                                                                                                                 \
    }                                                                                                                                                                                                                 \
                                                                                                                                                                                                                      \
    __VA_ARGS__* Set##Name(__VA_ARGS__*& Value) const                                                                                                                                                                 \
    {                                                                                                                                                                                                                 \
        if (Name##__Offset == -2)                                                                                                                                                                                     \
        {                                                                                                                                                                                                             \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x8010000);                                                                                                                                                \
            if (!Prop)                                                                                                                                                                                                \
                Name##__Offset = -1;                                                                                                                                                                                  \
            else                                                                                                                                                                                                      \
            {                                                                                                                                                                                                         \
                if (VersionInfo.FortniteVersion >= 12.10)                                                                                                                                                             \
                {                                                                                                                                                                                                     \
                    auto FieldClass = *(void**)(__int64(Prop) + 0x8);                                                                                                                                                 \
                    auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);                                                                                                                                       \
                    if ((FieldFlags & 0x8000000) != 0)                                                                                                                                                                \
                        Name##__Weak = true;                                                                                                                                                                          \
                }                                                                                                                                                                                                     \
                Name##__Offset = GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);                                                                                                                               \
            }                                                                                                                                                                                                         \
        }                                                                                                                                                                                                             \
        if (Name##__Weak)                                                                                                                                                                                             \
            GetFromOffset<TWeakObjectPtr<__VA_ARGS__>>(this, Name##__Offset) = Value;                                                                                                                                 \
        else                                                                                                                                                                                                          \
            GetFromOffset<__VA_ARGS__*>(this, Name##__Offset) = Value;                                                                                                                                                \
        return Value;                                                                                                                                                                                                 \
    }                                                                                                                                                                                                                 \
                                                                                                                                                                                                                      \
    __declspec(property(get = Get##Name, put = Set##Name)) __VA_ARGS__* Name;

inline std::vector<void (*)()> _HookFuncs;
inline std::vector<void (*)()> _PostLoadHookFuncs;
#define DefHookOg(_Rt, _Name, ...)                                                                                                                                                                                    \
    static inline _Rt (*_Name##OG)(##__VA_ARGS__);                                                                                                                                                                    \
    static _Rt _Name(##__VA_ARGS__);
#define DefUHookOg(_Name)                                                                                                                                                                                             \
    static inline void (*_Name##OG)(UObject*, FFrame&);                                                                                                                                                               \
    static void _Name(UObject*, FFrame&);
#define DefUHookOgRet(_Rt, _Name)                                                                                                                                                                                     \
    static inline void (*_Name##OG)(UObject*, FFrame&, _Rt*);                                                                                                                                                         \
    static void _Name(UObject*, FFrame&, _Rt*);
#ifdef CLIENT
#define InitHooks static void Hook();
#define InitPostLoadHooks static void PostLoadHook();
#else
#define InitHooks                                                                                                                                                                                                     \
    static void Hook();                                                                                                                                                                                               \
    static int _AddHook()                                                                                                                                                                                             \
    {                                                                                                                                                                                                                 \
        _HookFuncs.push_back(Hook);                                                                                                                                                                                   \
        return 0;                                                                                                                                                                                                     \
    };                                                                                                                                                                                                                \
    static inline auto _HookAdder = _AddHook();
#define InitPostLoadHooks                                                                                                                                                                                             \
    static void PostLoadHook();                                                                                                                                                                                       \
    static int _AddPostLoadHook()                                                                                                                                                                                     \
    {                                                                                                                                                                                                                 \
        _PostLoadHookFuncs.push_back(PostLoadHook);                                                                                                                                                                   \
        return 0;                                                                                                                                                                                                     \
    };                                                                                                                                                                                                                \
    static inline auto _PostLoadHookAdder = _AddPostLoadHook();
#endif
#define callOG(_Tr, _Fn, _Th, ...)                                                                                                                                                                                    \
    ([&]()                                                                                                                                                                                                            \
    {                                                                                                                                                                                                                 \
        _Fn->ExecFunction = _Th##_OG;                                                                                                                                                                                 \
        _Tr->_Th(##__VA_ARGS__);                                                                                                                                                                                      \
        _Fn->ExecFunction = _Th##_;                                                                                                                                                                                   \
    })()
#define callOGWithRet(_Tr, _Fn, _Th, ...)                                                                                                                                                                             \
    ([&]()                                                                                                                                                                                                            \
    {                                                                                                                                                                                                                 \
        _Fn->ExecFunction = _Th##_OG;                                                                                                                                                                                 \
        auto _Rt = _Tr->_Th(##__VA_ARGS__);                                                                                                                                                                           \
        _Fn->ExecFunction = _Th##_;                                                                                                                                                                                   \
        return _Rt;                                                                                                                                                                                                   \
    })()
