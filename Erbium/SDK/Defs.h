#pragma once
#define UCLASS_COMMON_MEMBERS(__Class)                                          \
	static const SDK::UClass* StaticClass()                                     \
	{                                                                           \
		static const SDK::UClass* _storage = nullptr;                           \
                                                                                \
		if (!_storage)                                                          \
	        _storage = SDK::FindClass(#__Class + 1);                            \
                                                                                \
		return _storage;                                                        \
	}                                                                           \
                                                                                \
	static const __Class* GetDefaultObj()                                       \
    {                                                                           \
         static const SDK::UObject* _storage = nullptr;                         \
                                                                                \
         if (!_storage)                                                         \
             _storage = StaticClass()->GetDefaultObj();                         \
                                                                                \
         return (const __Class*)_storage;                                       \
	}                                                                           \
                                                                                \
	static __Class* GetFirstInstance()                                          \
	{                                                                           \
		return (__Class*) SDK::TUObjectArray::FindFirstObject(#__Class + 1);    \
	}

#define USCRIPTSTRUCT_COMMON_MEMBERS(__Class)                                   \
	static const SDK::UStruct* StaticStruct()                                   \
	{                                                                           \
		static const SDK::UStruct* _storage = nullptr;                          \
                                                                                \
		if (!_storage)                                                          \
	        _storage = SDK::FindStruct(#__Class + 1);                           \
                                                                                \
		return _storage;                                                        \
	}                                                                           \
                                                                                \
	static const int32 Size()                                                   \
	{                                                                           \
		static int32 _size = -1;                                                \
                                                                                \
		if (_size == -1)                                                        \
            _size = StaticStruct()->GetPropertiesSize();                        \
                                                                                \
		return _size;                                                           \
	}                                                                           \
                                                                                \
    __Class& operator=(__Class& _Rhs)                                           \
    {                                                                           \
        __movsb((PBYTE)this, (const PBYTE)&_Rhs, Size());                       \
        return *this;                                                           \
    }                                                                           \
    static inline constexpr auto _StructName = #__Class + 1;

#define UENUM_COMMON_MEMBERS(__Class)                                           \
	static const SDK::UEnum* StaticEnum()                                       \
	{                                                                           \
		static const SDK::UEnum* _storage = nullptr;                            \
                                                                                \
		if (!_storage)                                                          \
	        _storage = SDK::FindEnum(#__Class);                                 \
                                                                                \
		return _storage;                                                        \
	}

#define DEFINE_PROP(Name, ...)                                                  \
    static inline int32 Name##__Offset = -2;                                    \
    __VA_ARGS__& Get##Name() const                                              \
    {                                                                           \
        if (Name##__Offset == -2)                                               \
            Name##__Offset = this->GetOffset(#Name);                            \
        return GetFromOffset<__VA_ARGS__>(this, Name##__Offset);                \
    }                                                                           \
                                                                                \
    bool Has##Name() const                                                      \
    {                                                                           \
        if (Name##__Offset == -2)                                               \
            Name##__Offset = this->GetOffset(#Name);                            \
        return Name##__Offset != -1;                                            \
    }                                                                           \
                                                                                \
    __VA_ARGS__& Set##Name(__VA_ARGS__ Value) const                             \
    {                                                                           \
        if (Name##__Offset == -2)                                               \
            Name##__Offset = this->GetOffset(#Name);                            \
        return GetFromOffset<__VA_ARGS__>(this, Name##__Offset) = Value;        \
    }                                                                           \
                                                                                \
    __declspec(property(get = Get##Name, put = Set##Name))                      \
    __VA_ARGS__ Name;

#define DEFINE_BITFIELD_PROP(Name)                                              \
    bool Get##Name() const                                                      \
    {                                                                           \
        return this->GetBitfield<#Name>();                                      \
    }                                                                           \
                                                                                \
    bool Has##Name() const                                                      \
    {                                                                           \
        return this->Has<#Name>();                                              \
    }                                                                           \
                                                                                \
    void Set##Name(bool Value) const                                            \
    {                                                                           \
        this->SetBitfield<#Name>(Value);                                        \
    }                                                                           \
                                                                                \
    __declspec(property(get = Get##Name, put = Set##Name))                      \
    bool Name;


#define DEFINE_STRUCT_PROP(Name, ...)                                           \
    static inline int32 Name##__Offset = -2;                                    \
    __VA_ARGS__& Get##Name() const                                              \
    {                                                                           \
        if (Name##__Offset == -2)                                               \
            Name##__Offset = StaticStruct()->GetOffset(#Name);                  \
        return GetFromOffset<__VA_ARGS__>(this, Name##__Offset);                \
    }                                                                           \
                                                                                \
    static bool Has##Name()                                                     \
    {                                                                           \
        if (Name##__Offset == -2)                                               \
            Name##__Offset = StaticStruct()->GetOffset(#Name);                  \
        return Name##__Offset != -1;                                            \
    }                                                                           \
                                                                                \
    __VA_ARGS__ Set##Name(__VA_ARGS__ Value) const                              \
    {                                                                           \
        if (Name##__Offset == -2)                                               \
            Name##__Offset = StaticStruct()->GetOffset(#Name);                  \
        return GetFromOffset<__VA_ARGS__>(this, Name##__Offset) = Value;        \
    }                                                                           \
                                                                                \
    __declspec(property(get = Get##Name, put = Set##Name))                      \
    __VA_ARGS__ Name;

#define DEFINE_ENUM_PROP(Name)                                                  \
    static inline __int64 Name##__Value = -1;                                   \
    static __int64 Get##Name()                                                  \
    {                                                                           \
        if (Name##__Value == -1)                                                \
            Name##__Value = StaticEnum()->GetValue(#Name);                      \
        return Name##__Value;                                                   \
    }                                                                           \
                                                                                \
    static bool Has##Name()                                                     \
    {                                                                           \
        return Get##Name() != -1;                                               \
    }                                                                           \


#define DEFINE_FUNC(Name, ...)                                                  \
    static inline UFunction* Name##__Ptr = nullptr;                             \
    /*void Name##_PE(void *Params)                                              \
    {                                                                           \
        this->Call<#Name>(Params);                                              \
    }*/                                                                         \
                                                                                \
    template <typename... Args>                                                 \
    __VA_ARGS__ Name(Args... Params)                                            \
    {                                                                           \
        if (!Name##__Ptr)                                                       \
            Name##__Ptr = GetFunction(#Name);                                   \
        return this->Call<__VA_ARGS__>(Name##__Ptr, Params...);                 \
    }                                                                           \
                                                                                \
    /*void Name##_LP(UEAllocatedVector<ParamPair> Params)                       \
    {                                                                           \
        this->Call<#Name>(Params);                                              \
    }*/

#define DEFINE_STATIC_FUNC(Name, ...)                                           \
    static inline UFunction* Name##__Ptr = nullptr;                             \
    /*static void Name##_PE(void *Params)                                       \
    {                                                                           \
        GetDefaultObj()->Call<#Name>(Params);                                   \
    }*/                                                                         \
                                                                                \
    template <typename... Args>                                                 \
    static __VA_ARGS__ Name(Args... Params)                                     \
    {                                                                           \
        if (!Name##__Ptr)                                                       \
            Name##__Ptr = GetDefaultObj()->GetFunction(#Name);                  \
        return GetDefaultObj()->Call<__VA_ARGS__>(Name##__Ptr, Params...);      \
    }                                                                           \
                                                                                \
    /*static void Name##_LP(UEAllocatedVector<SDK::ParamPair> Params)           \
    {                                                                           \
        GetDefaultObj()->Call<#Name>(Params);                                   \
    }*/