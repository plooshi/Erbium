#pragma once

#define GUESS_PROP_FLAGS(...) std::is_same_v<__VA_ARGS__, bool> ? 0x20000 : \
                             (std::is_same_v<__VA_ARGS__, int32> ? 0x80 : \
                             (std::is_same_v<__VA_ARGS__, float> ? 0x100 : \
                             (std::is_same_v<__VA_ARGS__, UClass*> ? 0x400 : \
                             (std::is_same_v<__VA_ARGS__, uint8> ? 0x1000000020040 : \
                             (std::is_base_of_v<UObject, std::remove_pointer_t<__VA_ARGS__>> ? 0x10000 : 0)))))

#define UCLASS_COMMON_MEMBERS(__Class)                                                                                                                               \
	static const SDK::UClass* StaticClass()                                                                                                                          \
	{                                                                                                                                                                \
		static const SDK::UClass* _storage = nullptr;                                                                                                                \
        static bool bInitialized = false;                                                                                                                            \
                                                                                                                                                                     \
		if (!bInitialized)                                                                                                                                           \
        {                                                                                                                                                            \
            bInitialized = true;                                                                                                                                     \
	        _storage = SDK::FindClass(#__Class + 1);                                                                                                                 \
        }                                                                                                                                                            \
                                                                                                                                                                     \
		return _storage;                                                                                                                                             \
	}                                                                                                                                                                \
                                                                                                                                                                     \
	static const __Class* GetDefaultObj()                                                                                                                            \
    {                                                                                                                                                                \
         static const SDK::UObject* _storage = nullptr;                                                                                                              \
        static bool bInitialized = false;                                                                                                                            \
                                                                                                                                                                     \
		if (!bInitialized)                                                                                                                                           \
        {                                                                                                                                                            \
            bInitialized = true;                                                                                                                                     \
            _storage = StaticClass()->GetDefaultObj();                                                                                                               \
        }                                                                                                                                                            \
                                                                                                                                                                     \
         return (const __Class*)_storage;                                                                                                                            \
	}                                                                                                                                                                \
                                                                                                                                                                     \
	static __Class* GetFirstInstance()                                                                                                                               \
	{                                                                                                                                                                \
		return (__Class*) SDK::TUObjectArray::FindFirstObject(#__Class + 1);                                                                                         \
	}

#define USCRIPTSTRUCT_COMMON_MEMBERS(__Class)                                                                                                                        \
	static const SDK::UStruct* StaticStruct()                                                                                                                        \
	{                                                                                                                                                                \
		static const SDK::UStruct* _storage = nullptr;                                                                                                               \
        static bool bInitialized = false;                                                                                                                            \
                                                                                                                                                                     \
		if (!bInitialized)                                                                                                                                           \
        {                                                                                                                                                            \
            bInitialized = true;                                                                                                                                     \
	        _storage = SDK::FindStruct(#__Class + 1);                                                                                                                \
        }                                                                                                                                                            \
                                                                                                                                                                     \
		return _storage;                                                                                                                                             \
	}                                                                                                                                                                \
                                                                                                                                                                     \
	static const int32 Size()                                                                                                                                        \
	{                                                                                                                                                                \
		static int32 _size = -1;                                                                                                                                     \
                                                                                                                                                                     \
		if (_size == -1)                                                                                                                                             \
            _size = StaticStruct()->GetPropertiesSize();                                                                                                             \
                                                                                                                                                                     \
		return _size;                                                                                                                                                \
	}                                                                                                                                                                \
                                                                                                                                                                     \
    __Class& operator=(__Class& _Rhs)                                                                                                                                \
    {                                                                                                                                                                \
        memcpy((PBYTE)this, (const PBYTE)&_Rhs, Size());                                                                                                             \
        return *this;                                                                                                                                                \
    }

#define UENUM_COMMON_MEMBERS(__Class)                                                                                                                                \
	static const SDK::UEnum* StaticEnum()                                                                                                                            \
	{                                                                                                                                                                \
		static const SDK::UEnum* _storage = nullptr;                                                                                                                 \
        static bool bInitialized = false;                                                                                                                            \
                                                                                                                                                                     \
		if (!bInitialized)                                                                                                                                           \
        {                                                                                                                                                            \
            bInitialized = true;                                                                                                                                     \
	        _storage = SDK::FindEnum(#__Class);                                                                                                                      \
        }                                                                                                                                                            \
                                                                                                                                                                     \
		return _storage;                                                                                                                                             \
	}

#define DEFINE_PROP(Name, ...)                                                                                                                                       \
    static inline int32 Name##__Offset = -2;                                                                                                                         \
    __VA_ARGS__& Get##Name() const                                                                                                                                   \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
            Name##__Offset = this->GetOffset(#Name, GUESS_PROP_FLAGS(__VA_ARGS__));                                                                                  \
        return GetFromOffset<__VA_ARGS__>(this, Name##__Offset);                                                                                                     \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    bool Has##Name() const                                                                                                                                           \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
            Name##__Offset = this->GetOffset(#Name, GUESS_PROP_FLAGS(__VA_ARGS__));                                                                                  \
        return Name##__Offset != -1;                                                                                                                                 \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __VA_ARGS__& Set##Name(__VA_ARGS__&& Value) const                                                                                                                \
    {                                                                                                                                                                \
        return Get##Name() = Value;                                                                                                                                  \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __VA_ARGS__& Set##Name(__VA_ARGS__& Value) const                                                                                                                 \
    {                                                                                                                                                                \
        return Get##Name() = Value;                                                                                                                                  \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __declspec(property(get = Get##Name, put = Set##Name))                                                                                                           \
    __VA_ARGS__ Name;

#define DEFINE_BITFIELD_PROP(Name)                                                                                                                                   \
    static inline int32 Name##__Offset = -2;                                                                                                                         \
    static inline uint8_t Name##__FieldMask = 0;                                                                                                                     \
    bool Get##Name() const                                                                                                                                           \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = GetProperty(#Name, 0x20000);                                                                                                                 \
            Name##__Offset = Prop ? GetFromOffset<uint32>(Prop, Offsets::Offset_Internal) : -1;                                                                      \
            Name##__FieldMask = Prop ? Prop->GetFieldMask() : 0;                                                                                                     \
        }                                                                                                                                                            \
        return (GetFromOffset<uint8_t>(this, Name##__Offset) & Name##__FieldMask) != 0;                                                                              \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    bool Has##Name() const                                                                                                                                           \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = GetProperty(#Name, 0x20000);                                                                                                                 \
            Name##__Offset = Prop ? GetFromOffset<uint32>(Prop, Offsets::Offset_Internal) : -1;                                                                      \
            Name##__FieldMask = Prop ? Prop->GetFieldMask() : 0;                                                                                                     \
        }                                                                                                                                                            \
        return Name##__Offset != -1;                                                                                                                                 \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    void Set##Name(bool Value) const                                                                                                                                 \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = GetProperty(#Name, 0x20000);                                                                                                                 \
            Name##__Offset = Prop ? GetFromOffset<uint32>(Prop, Offsets::Offset_Internal) : -1;                                                                      \
            Name##__FieldMask = Prop ? Prop->GetFieldMask() : 0;                                                                                                     \
        }                                                                                                                                                            \
        Value ? GetFromOffset<uint8_t>(this, Name##__Offset) |= Name##__FieldMask : GetFromOffset<uint8_t>(this, Name##__Offset) &= ~Name##__FieldMask;              \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __declspec(property(get = Get##Name, put = Set##Name))                                                                                                           \
    bool Name;


#define DEFINE_STRUCT_PROP(Name, ...)                                                                                                                                \
    static inline int32 Name##__Offset = -2;                                                                                                                         \
    __VA_ARGS__& Get##Name() const                                                                                                                                   \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
            Name##__Offset = StaticStruct()->GetOffset(#Name, GUESS_PROP_FLAGS(__VA_ARGS__));                                                                        \
        return GetFromOffset<__VA_ARGS__>(this, Name##__Offset);                                                                                                     \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    static bool Has##Name()                                                                                                                                          \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
            Name##__Offset = StaticStruct()->GetOffset(#Name, GUESS_PROP_FLAGS(__VA_ARGS__));                                                                        \
        return Name##__Offset != -1;                                                                                                                                 \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __VA_ARGS__& Set##Name(__VA_ARGS__&& Value) const                                                                                                                \
    {                                                                                                                                                                \
        return Get##Name() = Value;                                                                                                                                  \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __VA_ARGS__& Set##Name(__VA_ARGS__& Value) const                                                                                                                 \
    {                                                                                                                                                                \
        return Get##Name() = Value;                                                                                                                                  \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __declspec(property(get = Get##Name, put = Set##Name))                                                                                                           \
    __VA_ARGS__ Name;

#define DEFINE_STRUCT_BITFIELD_PROP(Name)                                                                                                                            \
    static inline int32 Name##__Offset = -2;                                                                                                                         \
    static inline uint8_t Name##__FieldMask = 0;                                                                                                                     \
    bool Get##Name() const                                                                                                                                           \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x20000);                                                                                                 \
            Name##__Offset = Prop ? GetFromOffset<uint32>(Prop, Offsets::Offset_Internal) : -1;                                                                      \
            Name##__FieldMask = Prop ? Prop->GetFieldMask() : 0;                                                                                                     \
        }                                                                                                                                                            \
        return (GetFromOffset<uint8_t>(this, Name##__Offset) & Name##__FieldMask) != 0;                                                                              \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    bool Has##Name() const                                                                                                                                           \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x20000);                                                                                                 \
            Name##__Offset = Prop ? GetFromOffset<uint32>(Prop, Offsets::Offset_Internal) : -1;                                                                      \
            Name##__FieldMask = Prop ? Prop->GetFieldMask() : 0;                                                                                                     \
        }                                                                                                                                                            \
        return Name##__Offset != -1;                                                                                                                                 \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    void Set##Name(bool Value) const                                                                                                                                 \
    {                                                                                                                                                                \
        if (Name##__Offset == -2)                                                                                                                                    \
        {                                                                                                                                                            \
            auto Prop = StaticStruct()->GetProperty(#Name, 0x20000);                                                                                                 \
            Name##__Offset = Prop ? GetFromOffset<uint32>(Prop, Offsets::Offset_Internal) : -1;                                                                      \
            Name##__FieldMask = Prop ? Prop->GetFieldMask() : 0;                                                                                                     \
        }                                                                                                                                                            \
        Value ? GetFromOffset<uint8_t>(this, Name##__Offset) |= Name##__FieldMask : GetFromOffset<uint8_t>(this, Name##__Offset) &= ~Name##__FieldMask;              \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    __declspec(property(get = Get##Name, put = Set##Name))                                                                                                           \
    bool Name;


#define DEFINE_ENUM_PROP(Name)                                                                                                                                       \
    static inline __int64 Name##__Value = -2;                                                                                                                        \
    static __int64 Get##Name()                                                                                                                                       \
    {                                                                                                                                                                \
        if (Name##__Value == -2)                                                                                                                                     \
            Name##__Value = StaticEnum()->GetValue(#Name);                                                                                                           \
        return Name##__Value;                                                                                                                                        \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    static bool Has##Name()                                                                                                                                          \
    {                                                                                                                                                                \
        return Get##Name() != -1;                                                                                                                                    \
    }                                                                                                                                                                \


#define DEFINE_FUNC(Name, ...)                                                                                                                                       \
    static inline UFunction* Name##__Ptr = nullptr;                                                                                                                  \
    static inline bool Name##__Initialized = false;                                                                                                                  \
    /*void Name##_PE(void *Params)                                                                                                                                   \
    {                                                                                                                                                                \
        this->Call<#Name>(Params);                                                                                                                                   \
    }*/                                                                                                                                                              \
                                                                                                                                                                     \
    template <typename... Args>                                                                                                                                      \
    __VA_ARGS__ Name(Args&&... Params) const                                                                                                                         \
    {                                                                                                                                                                \
        if (!Name##__Initialized)                                                                                                                                    \
        {                                                                                                                                                            \
            Name##__Initialized = true;                                                                                                                              \
            Name##__Ptr = GetFunction(#Name);                                                                                                                        \
        }                                                                                                                                                            \
        return this->Call<__VA_ARGS__>(Name##__Ptr, Params...);                                                                                                      \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    /*void Name##_LP(UEAllocatedVector<ParamPair> Params)                                                                                                            \
    {                                                                                                                                                                \
        this->Call<#Name>(Params);                                                                                                                                   \
    }*/

#define DEFINE_STATIC_FUNC(Name, ...)                                                                                                                                \
    static inline UFunction* Name##__Ptr = nullptr;                                                                                                                  \
    static inline bool Name##__Initialized = false;                                                                                                                  \
    /*static void Name##_PE(void *Params)                                                                                                                            \
    {                                                                                                                                                                \
        GetDefaultObj()->Call<#Name>(Params);                                                                                                                        \
    }*/                                                                                                                                                              \
                                                                                                                                                                     \
    template <typename... Args>                                                                                                                                      \
    static __VA_ARGS__ Name(Args&&... Params)                                                                                                                        \
    {                                                                                                                                                                \
        auto DefaultObj = GetDefaultObj();                                                                                                                           \
        if (!Name##__Initialized)                                                                                                                                    \
        {                                                                                                                                                            \
            Name##__Initialized = true;                                                                                                                              \
            Name##__Ptr = DefaultObj->GetFunction(#Name);                                                                                                            \
        }                                                                                                                                                            \
        return DefaultObj->Call<__VA_ARGS__>(Name##__Ptr, Params...);                                                                                                \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    /*static void Name##_LP(UEAllocatedVector<SDK::ParamPair> Params)                                                                                                \
    {                                                                                                                                                                \
        GetDefaultObj()->Call<#Name>(Params);                                                                                                                        \
    }*/
