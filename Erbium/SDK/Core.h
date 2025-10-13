#pragma once
using namespace UC;

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
inline uint64_t ImageBase = *(uint64_t*)(__readgsqword(0x60) + 0x10);

namespace SDK
{
	class FName 
	{
	public:
		int32 ComparisonIndex;
		int32 Number;

		bool IsValid() const;
		UEAllocatedString ToString() const;
		UEAllocatedString ToSDKString() const;
		UEAllocatedWString ToWString() const;
		UEAllocatedWString ToSDKWString() const;
		bool operator==(const FName& Other) const;
		bool operator!=(const FName& Other) const;
		bool operator<(const FName& Other) const;
		operator bool() const;
	};

	template <typename _Ot>
	__forceinline _Ot& GetFromOffset(void* Obj, uint32 Offset) {
		return *(_Ot*)(__int64(Obj) + Offset);
	}

	template <typename _Ot>
	__forceinline _Ot& GetFromOffset(const void* Obj, uint32 Offset) {
		return *(_Ot*)(__int64(Obj) + Offset);
	}

	template <typename _Ot>
	__forceinline _Ot* GetPtrFromOffset(const void* Obj, uint32 Offset) {
		return (_Ot*)(__int64(Obj) + Offset);
	}

	class FStructBaseChain
	{
	public:
		FStructBaseChain(const FStructBaseChain&) = delete;
		FStructBaseChain& operator=(const FStructBaseChain&) = delete;

		__forceinline bool IsChildOfUsingStructArray(const FStructBaseChain& Parent) const
		{
			int32 NumParentStructBasesInChainMinusOne = Parent.NumStructBasesInChainMinusOne;
			return NumParentStructBasesInChainMinusOne <= NumStructBasesInChainMinusOne && StructBaseChainArray[NumParentStructBasesInChainMinusOne] == &Parent;
		}

	private:
		FStructBaseChain** StructBaseChainArray;
		int32 NumStructBasesInChainMinusOne;

		friend class UStruct;
	};

	class UObject 
	{
	public:
		void** Vft;
		int32 ObjectFlags;
		int32 Index;
		class UClass* Class;
		class FName Name;
		UObject* Outer;

	public:
		const class UField* GetProperty(const char* Name, uint64_t CastFlags = 0) const;
		bool IsDefaultObject() const;
		uint32 GetOffset(const char* Name, uint64_t CastFlags = 0) const;
		bool IsA(const class UClass* Clss) const;

		template <class T>
		bool IsA() const
		{
			return IsA(T::StaticClass());
		}

		template <class T>
		T* Cast(const class UClass* Clss = T::StaticClass()) const
		{
			return IsA(Clss) ? (T*)this : nullptr;
		}

		class UFunction* GetFunction(const char* Name) const;
		void ProcessEvent(class UFunction* Function, void* Params) const;

		template <typename Ret = void, typename... Args>
		Ret Call(UFunction* Function, Args... args) const;

		static const UClass* StaticClass();

		const class IInterface* GetInterface(const class UClass*) const;
		
		void AddToRoot() const;
	};

	class UField : public UObject
	{
	public:
		const UField* FField_GetNext() const;
		FName& FField_GetName() const;
		const UField* GetNext() const;
		FName& GetName() const;
		const uint8 GetFieldMask() const;
	};

	class UStruct : public UField 
	{
	public:
		const UStruct* GetSuper() const;
		const int32 GetPropertiesSize() const;
		const UField* GetChildProperties() const;
		const UField* GetChildren() const;
		const UField* GetProperty(const char* Name, uint64_t CastFlags = 0) const;
		uint32_t GetOffset(const char* Name, uint64_t CastFlags = 0) const;
	};

	class UClass : public UStruct 
	{
	public:
		uint64_t GetCastFlags() const;
		static const UClass* StaticClass();

		UObject* GetDefaultObj() const;
	};

	class UFunction : public UStruct 
	{
	public:
		void*& GetNativeFunc();
		void SetNativeFunc(void* NewFunc);

		__declspec(property(get = GetNativeFunc, put = SetNativeFunc))
		void* ExecFunction;

		void* GetImpl();
		void Call(const UObject* obj, void* Params);
		void operator()(const UObject* obj, void* Params);
		uint32 GetVTableIndex();
		static const UClass* StaticClass();

		struct Param 
		{
			//UEAllocatedString Name;
			uint32 Offset;
			uint64 PropertyFlags;
			uint32 ElementSize;
		};

		struct ParamNamed : public Param
		{
			UEAllocatedString Name;
		};

		UEAllocatedVector<Param> GetParams();
		UEAllocatedVector<ParamNamed> GetParamsNamed();
	};

	template <typename Ret, typename... Args>
	Ret UObject::Call(UFunction* Function, Args... args) const
	{

		if (!Function)
			return Ret();

		// fast paths
		if constexpr (sizeof...(args) == 0 && std::is_void_v<Ret>)
			return ProcessEvent(Function, nullptr);

		if constexpr (sizeof...(args) == 1 && std::is_void_v<Ret>)
			return ProcessEvent(Function, &args...);

		if constexpr (sizeof...(args) == 0 && !std::is_void_v<Ret>)
		{
			Ret ret{};

			ProcessEvent(Function, &ret);

			return ret;
		}

		auto PropertiesSize = Function->GetPropertiesSize();
		auto Params = Function->GetParams();
		auto Mem = FMemory::Malloc(PropertiesSize);
		__stosb((PBYTE)Mem, 0, PropertiesSize);

		size_t i = 0;
		([&] 
		{
			if (i >= Params.size())
				return;

			auto& Param = Params[i];

			if (((Param.PropertyFlags & 0x100) != 0 && (Param.PropertyFlags & 0x8000000) == 0) || (Param.PropertyFlags & 0x400) != 0)
			{
				i++;
				return;
			}

			const auto& Arg = args;

			if constexpr (std::is_same_v<decltype(args), bool> || std::is_same_v<decltype(args), char> || std::is_same_v<decltype(args), uint8_t> || std::is_same_v<decltype(args), short> || std::is_same_v<decltype(args), uint16_t> || std::is_same_v<decltype(args), int> || std::is_same_v<decltype(args), uint32_t> || std::is_same_v<decltype(args), int64_t> || std::is_same_v<decltype(args), uint64_t> || std::is_same_v<decltype(args), float> || std::is_same_v<decltype(args), double> || std::is_pointer_v<decltype(args)>)
				*(decltype(args)*)(__int64(Mem) + Param.Offset) = Arg;
			else
				__movsb(PBYTE(__int64(Mem) + Param.Offset), (const PBYTE)&Arg, Param.ElementSize);
			i++;
		}(), ...);

		ProcessEvent(Function, Mem);

		i = 0;
		([&] {
			if (i >= Params.size())
				return;

			auto& Param = Params[i];

			if (((Param.PropertyFlags & 0x100) == 0 && (Param.PropertyFlags & 0x8000000) == 0) || (Param.PropertyFlags & 0x400) != 0)
			{
				i++;
				return;
			}

			const auto& Arg = args;

			if constexpr (std::is_pointer_v<decltype(args)>)
			{
				if (Arg != nullptr)
					if constexpr (std::is_same_v<std::remove_pointer_t<decltype(args)>, bool> || std::is_same_v<std::remove_pointer_t<decltype(args)>, char> || std::is_same_v<std::remove_pointer_t<decltype(args)>, uint8_t> || std::is_same_v<std::remove_pointer_t<decltype(args)>, short> || std::is_same_v<std::remove_pointer_t<decltype(args)>, uint16_t> || std::is_same_v<std::remove_pointer_t<decltype(args)>, int> || std::is_same_v<std::remove_pointer_t<decltype(args)>, uint32_t> || std::is_same_v<std::remove_pointer_t<decltype(args)>, int64_t> || std::is_same_v<std::remove_pointer_t<decltype(args)>, uint64_t> || std::is_same_v<std::remove_pointer_t<decltype(args)>, float> || std::is_same_v<std::remove_pointer_t<decltype(args)>, double> || std::is_pointer_v<std::remove_pointer_t<decltype(args)>>)
						*(decltype(args))Arg = *(decltype(args))(__int64(Mem) + Param.Offset);
					else
						__movsb((PBYTE)Arg, (const PBYTE)(__int64(Mem) + Param.Offset), Param.ElementSize);
			}
			else if constexpr (std::is_reference_v<decltype(args)>)
			{
				if ((Param.PropertyFlags & 0x2) != 0)
					if constexpr (std::is_same_v<decltype(args), bool> || std::is_same_v<decltype(args), char> || std::is_same_v<decltype(args), uint8_t> || std::is_same_v<decltype(args), short> || std::is_same_v<decltype(args), uint16_t> || std::is_same_v<decltype(args), int> || std::is_same_v<decltype(args), uint32_t> || std::is_same_v<decltype(args), int64_t> || std::is_same_v<decltype(args), uint64_t> || std::is_same_v<decltype(args), float> || std::is_same_v<decltype(args), double> || std::is_pointer_v<decltype(args)>)
						Arg = *(decltype(args)*)(__int64(Mem) + Param.Offset);
					else
						__movsb((PBYTE)&Arg, (const PBYTE)(__int64(Mem) + Param.Offset), Param.ElementSize);
			}
			i++;
		}(), ...);

		if constexpr (!std::is_void_v<Ret>)
		{
			Ret ret{};
			for (auto& Param : Params)
			{
				if ((Param.PropertyFlags & 0x400) == 0)
					continue;

				if constexpr (std::is_same_v<Ret, bool> || std::is_same_v<Ret, char> || std::is_same_v<Ret, uint8_t> || std::is_same_v<Ret, short> || std::is_same_v<Ret, uint16_t> || std::is_same_v<Ret, int> || std::is_same_v<Ret, uint32_t> || std::is_same_v<Ret, int64_t> || std::is_same_v<Ret, uint64_t> || std::is_same_v<Ret, float> || std::is_same_v<Ret, double> || std::is_pointer_v<Ret>)
					ret = *(Ret*)(__int64(Mem) + Param.Offset);
				else
					__movsb((PBYTE)&ret, (const PBYTE)(__int64(Mem) + Param.Offset), Param.ElementSize);
				break;
			}

			FMemory::Free(Mem);
			return ret;
		}

		FMemory::Free(Mem);
	}


	struct FUObjectItem final
	{
	public:
		const class UObject* Object;
		int32 Flags;
		int32 ClusterRootIndex;
		int32 SerialNumber;
	};

	class TUObjectArrayUnchunked
	{
	private:
		const FUObjectItem* Objects;
		const int32 MaxElements;
		const int32 NumElements;

	public:
		inline int Num() const;
		inline int Max() const;
		inline const FUObjectItem* GetItemByIndex(const int32 Index) const;
		const UObject* GetObjectByIndex(const int32 Index);
		__declspec(noinline) const UObject* FindObject(const char* Name, uint64 TypeFlags = 0, const UClass* TargetClass = nullptr);
	};

	class TUObjectArrayChunked
	{
	private:
		const FUObjectItem** Objects;
		const FUObjectItem* PreAllocatedObjects;
		const int32 MaxElements;
		const int32 NumElements;
		const int32 MaxChunks;
		const int32 NumChunks;

	public:
		inline int Num() const;
		inline int Max() const;
		inline const FUObjectItem* GetItemByIndex(const int32 Index) const;
		const UObject* GetObjectByIndex(const int32 Index);
		__declspec(noinline) const UObject* FindObject(const char* Name, uint64 TypeFlags = 0, const UClass* TargetClass = nullptr);
	};

	class TUObjectArray {
	public:
		static const int32 Num();
		static const int32 Max();
		static const FUObjectItem* GetItemByIndex(const int32 Index);
		static const UObject* GetObjectByIndex(const int32 Index);
		static const UObject* FindObject(const char* Name, uint64 TypeFlags = 0, const UClass* TargetClass = nullptr);

		template <typename _Et = UObject>
		static const _Et* FindObject(const char* Name, uint64 TypeFlags = 0, const UClass* TargetClass = _Et::StaticClass())
		{
			return (const _Et*)FindObject(Name, TypeFlags, TargetClass);
		}

		template <typename _Et = UObject>
		static const _Et* FindObject(const std::string& Name, uint64 TypeFlags = 0, const UClass* TargetClass = _Et::StaticClass())
		{
			return FindObject<_Et>(Name.c_str(), TypeFlags, TargetClass);
		}

		static const UObject* FindFirstObject(const char* Name);
	};

	const UClass* FindClass(const char* Name);
	const UObject* DefaultObjImpl(const char* Name);
	const UObject* DefaultObjImpl(const UClass* TargetClass, const char* Name);
	const UStruct* FindStruct(const char* Name);
	const class UEnum* FindEnum(const char* Name);

	class UEnum : public UField
	{
	public:
		int64 GetValue(const char* Member) const;
	};

	class FWeakObjectPtr
	{
	public:
		int32 ObjectIndex;
		int32 ObjectSerialNumber;

		FWeakObjectPtr(int32 Index = 0, int32 SerialNumber = 0)
			: ObjectIndex(Index), ObjectSerialNumber(SerialNumber)
		{
		}

		FWeakObjectPtr(const UObject* Object);
		FWeakObjectPtr(UObject* Object);
	public:
		const UObject* Get() const;
		const UObject* operator->() const;
		bool operator==(const FWeakObjectPtr& Other) const;
		bool operator!=(const FWeakObjectPtr& Other) const;
		bool operator==(const class UObject* Other) const;
		bool operator!=(const class UObject* Other) const;
	};

	template<typename UEType>
	class TWeakObjectPtr : public FWeakObjectPtr
	{
	public:
		TWeakObjectPtr(int32 Index = 0, int32 SerialNumber = 0)
			: FWeakObjectPtr(Index, SerialNumber)
		{
		}

		TWeakObjectPtr(const UEType* Obj)
			: FWeakObjectPtr(Obj)
		{
		}

		TWeakObjectPtr(UEType* Obj)
			: FWeakObjectPtr(Obj)
		{
		}

		UEType* Get() const
		{
			return (UEType*) FWeakObjectPtr::Get();
		}

		UEType* operator->() const
		{
			return (UEType*) FWeakObjectPtr::Get();
		}
	};

	template<typename TObjectID>
	class TPersistentObjectPtr
	{
	public:
		FWeakObjectPtr                                WeakPtr;
		int32                                         TagAtLastTest;
		TObjectID                                     ObjectID;

	public:
		const UObject* Get() const;
		const UObject* operator->() const;
	};

	struct FSoftObjectPath
	{
	public:
		class FName AssetPathName;
		class FString SubPathString;
	};


	const UObject* StaticFindObject(const wchar_t* ObjectPath, const UClass* Class);
	const UObject* StaticLoadObject(const wchar_t* ObjectPath, const UClass* InClass, UObject* Outer = nullptr);
	const UObject* FindObject(const wchar_t* ObjectPath, const UClass* Class);

	class FSoftObjectPtr : public TPersistentObjectPtr<FSoftObjectPath>
	{
	public:
		const UObject* InternalGet(const UClass* Class);
	};

	template<typename UEType>
	class TSoftObjectPtr : public FSoftObjectPtr
	{
	public:
		TSoftObjectPtr() {}

		TSoftObjectPtr(UEType* Obj)
		{
			WeakPtr = FWeakObjectPtr(Obj);
			ObjectID.AssetPathName = FName(0);
		}

		const UEType* Get()
		{
			return (UEType*)InternalGet(UEType::StaticClass());
			//return static_cast<const UEType*>(TPersistentObjectPtr::Get());
		}
		const UEType* operator->()
		{
			return Get();
		}
		operator const UEType* ()
		{
			return Get();
		}
	};

	template<typename UEType>
	class TSoftClassPtr : public FSoftObjectPtr
	{
	public:
		TSoftClassPtr() {}

		TSoftClassPtr(UClass* Obj)
		{
			WeakPtr = FWeakObjectPtr(Obj);
			ObjectID.AssetPathName = FName(0);
		}

		UClass* Get()
		{
			return (UEType*)InternalGet(UClass::StaticClass());
			//return static_cast<UClass*>(TPersistentObjectPtr::Get());
		}
		UClass* operator->()
		{
			return Get();
		}
		operator const UClass* ()
		{
			return Get();
		}
	};

	class IInterface : public UObject
	{
	};

	class FScriptInterface
	{
	public:
		UObject* ObjectPointer = nullptr;
		IInterface* InterfacePointer = nullptr;
	};

	template<class InterfaceType>
	class TScriptInterface : public FScriptInterface
	{
	};
}
