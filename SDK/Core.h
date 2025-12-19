#pragma once
#include <tuple>
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

		FName(int32 InComparisonIndex = 0, int32 InNumber = 0)
			: ComparisonIndex(InComparisonIndex)
		{
			if (VersionInfo.FortniteVersion < 20.00)
				Number = InNumber;
		}

		FName(const wchar_t* String)
		{
			auto& FName__Ctor = (void(*&)(FName*, const wchar_t*, int)) Offsets::FNameConstructor;

			FName__Ctor(this, String, 1);
		}

		FName(UEAllocatedWString String)
		{
			auto& FName__Ctor = (void(*&)(FName*, const wchar_t*, int)) Offsets::FNameConstructor;

			FName__Ctor(this, String.c_str(), 1);
		}

		FName(FString String)
		{
			auto& FName__Ctor = (void(*&)(FName*, const wchar_t*, int)) Offsets::FNameConstructor;

			FName__Ctor(this, String.CStr(), 1);
		}


		bool IsValid() const
		{
			return ComparisonIndex > 0;
		}

		UEAllocatedString ToString() const
		{
			if (!Offsets::AppendString)
			{
				if (IsBadReadPtr((void*)this))
					return "";
				FString TempString(1024);

				auto ToString = (void(*&)(const FName*, FString&)) Offsets::ToString;
				ToString(this, TempString);

				UEAllocatedString OutputString = TempString.ToString();
				TempString.Free();

				return OutputString;
			}

			thread_local FString TempString(1024);

			auto AppendString = (void(*&)(const FName*, FString&)) Offsets::AppendString;
			AppendString(this, TempString);

			UEAllocatedString OutputString = TempString.ToString();
			TempString.Clear();

			return OutputString;
		}

		UEAllocatedString ToSDKString() const
		{
			UEAllocatedString OutputString = ToString();

			size_t pos = OutputString.rfind('/');

			if (pos == UEAllocatedString::npos)
				return OutputString;

			return OutputString.substr(pos + 1);
		}

		UEAllocatedWString ToWString() const
		{
			if (!Offsets::AppendString)
			{
				if (IsBadReadPtr((void*)this))
					return L"";
				FString TempString(1024);

				auto ToString = (void(*&)(const FName*, FString&)) Offsets::ToString;
				ToString(this, TempString);

				UEAllocatedWString OutputString = TempString.ToWString();
				TempString.Free();

				return OutputString;
			}

			thread_local FString TempString(1024);

			auto AppendString = (void(*&)(const FName*, FString&)) Offsets::AppendString;
			AppendString(this, TempString);

			UEAllocatedWString OutputString = TempString.ToWString();
			TempString.Clear();

			return OutputString;
		}


		UEAllocatedWString ToSDKWString() const
		{
			UEAllocatedWString OutputString = ToWString();

			size_t pos = OutputString.rfind('/');

			if (pos == UEAllocatedWString::npos)
				return OutputString;

			return OutputString.substr(pos + 1);
		}

		bool operator==(const FName& Other) const
		{
			return ComparisonIndex == Other.ComparisonIndex && (VersionInfo.FortniteVersion >= 20.00 || Number == Other.Number);
		}
		bool operator!=(const FName& Other) const
		{
			return ComparisonIndex != Other.ComparisonIndex || (VersionInfo.FortniteVersion >= 20.00 ? false : Number != Other.Number);
		}

		bool operator<(const FName& Other) const
		{
			return ComparisonIndex == Other.ComparisonIndex ? (VersionInfo.FortniteVersion < 20.00 && Number < Other.Number) : ComparisonIndex < Other.ComparisonIndex;
		}

		operator bool() const {
			return IsValid();
		}
	};

	class ParamPair
	{
	public:
		UEAllocatedString ParamName;
		void* Value;

		template <typename ValueType>
		ParamPair(UEAllocatedString Name, ValueType Val)
		{
			ParamName = Name;
			// really scuffed way to make this work, just using & gives the same address for each param
			Value = FMemory::Malloc(sizeof(ValueType));
			memcpy(Value, &Val, sizeof(ValueType));
		}
	};

	template <typename _Ot>
	__forceinline _Ot& GetFromOffset(void* Obj, uint32 Offset)
	{
		return *(_Ot*)(__int64(Obj) + Offset);
	}

	template <typename _Ot>
	__forceinline _Ot& GetFromOffset(const void* Obj, uint32 Offset)
	{
		return *(_Ot*)(__int64(Obj) + Offset);
	}

	template <typename _Ot>
	__forceinline _Ot* GetPtrFromOffset(const void* Obj, uint32 Offset)
	{
		return (_Ot*)(__int64(Obj) + Offset);
	}

	class FStructBaseChain
	{
	public:
		FStructBaseChain()
			: StructBaseChainArray(nullptr)
			, NumStructBasesInChainMinusOne(-1)
		{
		}
		~FStructBaseChain()
		{
			delete[] StructBaseChainArray;
		}

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

		bool IsDefaultObject() const
		{
			return ObjectFlags & 0x10;
		}

		uint32 GetOffset(const char* Name, uint64_t CastFlags = 0) const
		{
			auto Prop = GetProperty(Name, CastFlags);
			if (!Prop) return -1;
			return GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);
		}

		bool IsA(const class UClass* Clss) const
		{
			if (!this || !Clss)
				return false;

			if (VersionInfo.EngineVersion >= 4.22)
			{
				auto& BaseChain = GetFromOffset<FStructBaseChain>(Class, 0x30);
				auto& BaseChainOther = GetFromOffset<FStructBaseChain>(Clss, 0x30);

				return BaseChain.IsChildOfUsingStructArray(BaseChainOther);
			}

			for (auto _Clss = Class; _Clss; _Clss = GetFromOffset<UClass*>(_Clss, 0x30))
			{
				if (_Clss == Clss) return true;
			}

			return false;
		}

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
		class UFunction* GetFunction(FName Name) const;

		void ProcessEvent(class UFunction* Function, void* Params) const
		{
			((void(*&)(const UObject*, class UFunction*, void*)) Vft[Offsets::ProcessEventVft])(this, Function, Params);
		}

		template <typename Ret = void, typename... Args>
		Ret Call(UFunction* Function, Args&&... args) const;

		static const UClass* StaticClass();

		const class IInterface* GetInterface(const class UClass*) const;

		void AddToRoot() const;
	};

	class UField : public UObject
	{
	public:
		const UField* FField_GetNext() const
		{
			return GetFromOffset<UField*>(this, Offsets::FField_Next);
		}

		FName& FField_GetName() const
		{
			return GetFromOffset<FName>(this, Offsets::FField_Name);
		}

		const UField* GetNext() const
		{
			return GetFromOffset<UField*>(this, 0x28);
		}

		FName& GetName() const
		{
			return GetFromOffset<FName>(this, 0x18);
		}

		const uint8 GetFieldMask() const
		{
			return GetFromOffset<uint8>(this, Offsets::FieldMask);
		}
	};

	class UStruct : public UField
	{
	public:
		const UStruct* GetSuper() const
		{
			return GetFromOffset<UStruct*>(this, Offsets::Super);
		}

		const int32 GetPropertiesSize() const
		{
			return GetFromOffset<int32>(this, Offsets::PropertiesSize);
		}

		const UField* GetChildProperties() const
		{
			return GetFromOffset<UField*>(this, 0x50);
		}

		const UField* GetChildren() const
		{
			return GetFromOffset<UField*>(this, Offsets::Children);
		}


		const UField* GetProperty(const char* Name, uint64_t CastFlags = 0) const;

		uint32_t GetOffset(const char* Name, uint64_t CastFlags = 0) const
		{
			auto Prop = GetProperty(Name, CastFlags);
			if (!Prop)
				return -1;

			return GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);
		}
	};

	class UClass : public UStruct
	{
	public:
		uint64_t GetCastFlags() const;
		static const UClass* StaticClass();

		UObject* GetDefaultObj() const;
	};

	__declspec(noinline) inline const UField* UStruct::GetProperty(const char* Name, uint64_t CastFlags) const
	{
		UEAllocatedString s = Name;
		UEAllocatedWString ws(s.begin(), s.end());
		auto PropName = FName(ws);

		for (const UStruct* Clss = this; Clss; Clss = (const UStruct*)Clss->GetSuper())
		{
			if (VersionInfo.FortniteVersion >= 12.10)
			{
				for (const UField* Prop = Clss->GetChildProperties(); Prop; Prop = Prop->FField_GetNext())
				{
					if (CastFlags != 0)
					{
						auto FieldClass = *(void**)(__int64(Prop) + 0x8);
						auto FieldFlags = *(uint64_t*)(__int64(FieldClass) + 0x10);

						if ((FieldFlags & CastFlags) == 0)
							continue;
					}

					if (Prop->FField_GetName() == PropName)
						return Prop;
				}
			}
			else
			{
				for (const UField* Prop = Clss->GetChildren(); Prop; Prop = Prop->GetNext())
				{
					if ((CastFlags == 0 || Prop->Class->GetCastFlags() & CastFlags) && Prop->GetName() == PropName)
						return Prop;
				}
			}
		}

		return nullptr;
	}

	inline const UField* UObject::GetProperty(const char* Name, uint64_t CastFlags) const
	{
		return Class->GetProperty(Name, CastFlags);
	}

	__declspec(noinline) inline UFunction* UObject::GetFunction(const char* Name) const
	{
		UEAllocatedString s = Name;
		UEAllocatedWString ws(s.begin(), s.end());
		auto PropName = FName(ws);

		for (const UStruct* Clss = Class; Clss; Clss = (const UStruct*)Clss->GetSuper())
			for (const UField* Prop = Clss->GetChildren(); Prop; Prop = Prop->GetNext())
				if (Prop->Class->GetCastFlags() & 0x80000 && Prop->GetName() == PropName)
					return (UFunction*)Prop;

		return nullptr;
		//return (UFunction*)GetProperty(Name, 0x80000);
	}


	__declspec(noinline) inline UFunction* UObject::GetFunction(FName Name) const
	{
		for (const UStruct* Clss = Class; Clss; Clss = (const UStruct*)Clss->GetSuper())
			for (const UField* Prop = Clss->GetChildren(); Prop; Prop = Prop->GetNext())
				if (Prop->Class->GetCastFlags() & 0x80000 && Prop->GetName() == Name)
					return (UFunction*)Prop;

		return nullptr;
		//return (UFunction*)GetProperty(Name, 0x80000);
	}

	class UFunction : public UStruct
	{
	public:
		void*& GetNativeFunc() const
		{
			return GetFromOffset<void*>(this, Offsets::ExecFunction);
		}

		void SetNativeFunc(void* NewFunc) const
		{
			GetNativeFunc() = NewFunc;
		}

		__declspec(property(get = GetNativeFunc, put = SetNativeFunc))
			void* ExecFunction;


		void* GetImpl() const
		{
			if (!this)
				return nullptr;

			auto setnzAddr = Memcury::Scanner(GetNativeFunc()).ScanFor({ 0x0F, 0x95 }).Get();

			for (int i = 0; i < 0x200; i++)
			{
				auto Ptr = (uint8_t*)(setnzAddr + i);

				if (*Ptr == 0xe9 || *Ptr == 0xe8)
					return Memcury::Scanner(Ptr).RelativeOffset(1).GetAs<void*>();
			}
			return nullptr;

		}

		void Call(const UObject* obj, void* Params)
		{
			if (this)
				obj->ProcessEvent(this, Params);
		}

		void Call(const UObject* obj, UEAllocatedVector<ParamPair> Params)
		{
			//if (this) 
				//obj->ProcessEvent(this, CreateParams(Params));
		}

		void operator()(const UObject* obj, void* Params)
		{
			return Call(obj, Params);
		}

		uint32 GetVTableIndex() const
		{
			if (!this)
				return -1;

			auto ValidateName = Name.ToString() + "_Validate";
			auto ValidateRef = Memcury::Scanner::FindStringRef(UEAllocatedWString(ValidateName.begin(), ValidateName.end()).c_str(), false);

			auto Addr = ValidateRef.Get();

			if (!Addr)
				Addr = Memcury::Scanner(GetNativeFunc()).ScanFor({ 0x0F, 0x95 }).Get();

			if (Addr)
				for (int i = 0; i < 2000; i++)
				{
					if (*((uint8*)Addr + i) == 0xFF && (*((uint8*)Addr + i + 1) == 0x90 || *((uint8*)Addr + i + 1) == 0x93 || *((uint8*)Addr + i + 1) == 0xA0))
					{
						auto VTIndex = *(uint32_t*)(Addr + i + 2);

						return VTIndex / 8;
					}
				}

			return -1;
		}

		static const UClass* StaticClass();

		struct Param
		{
			//UEAllocatedString Name;
			uint32 Offset;
			uint64 PropertyFlags;
			uint32 ElementSize;
		};
		class Params
		{
		public:
			UEAllocatedVector<Param> NameOffsetMap;
			uint32 Size;
		};


		struct ParamNamed
		{
			UEAllocatedString Name;
			uint32 Offset;
			uint64 PropertyFlags;
			uint32 ElementSize;
		};
		class ParamsNamed
		{
		public:
			UEAllocatedVector<ParamNamed> NameOffsetMap;
			uint32 Size;
		};

		Params GetParams() const
		{
			Params p{};

			if (VersionInfo.FortniteVersion >= 12.10)
				for (const UField* _Pr = GetChildProperties(); _Pr; _Pr = _Pr->FField_GetNext())
					p.NameOffsetMap.push_back({ GetFromOffset<uint32>(_Pr, Offsets::Offset_Internal), GetFromOffset<uint64>(_Pr, Offsets::PropertyFlags), GetFromOffset<uint32>(_Pr, Offsets::ElementSize) });
			else
				for (const UField* _Pr = GetChildren(); _Pr; _Pr = _Pr->GetNext())
					p.NameOffsetMap.push_back({ GetFromOffset<uint32>(_Pr, Offsets::Offset_Internal), GetFromOffset<uint64>(_Pr, Offsets::PropertyFlags), GetFromOffset<uint32>(_Pr, Offsets::ElementSize) });

			p.Size = GetPropertiesSize();
			return p;
		}


		ParamsNamed GetParamsNamed() const
		{
			ParamsNamed p{};

			if (VersionInfo.FortniteVersion >= 12.10)
				for (const UField* _Pr = GetChildProperties(); _Pr; _Pr = _Pr->FField_GetNext())
					p.NameOffsetMap.push_back({ _Pr->FField_GetName().ToSDKString(), GetFromOffset<uint32>(_Pr, Offsets::Offset_Internal), GetFromOffset<uint64>(_Pr, Offsets::PropertyFlags), GetFromOffset<uint32>(_Pr, Offsets::ElementSize) });
			else
				for (const UField* _Pr = GetChildren(); _Pr; _Pr = _Pr->GetNext())
					p.NameOffsetMap.push_back({ _Pr->GetName().ToSDKString(), GetFromOffset<uint32>(_Pr, Offsets::Offset_Internal), GetFromOffset<uint64>(_Pr, Offsets::PropertyFlags), GetFromOffset<uint32>(_Pr, Offsets::ElementSize) });

			p.Size = GetPropertiesSize();
			return p;
		}
	};

	template <typename Ret, typename... Args>
	Ret UObject::Call(UFunction* Function, Args&&... args) const
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

		auto Params = Function->GetParams();
		auto Mem = FMemory::Malloc(Params.Size);
		memset((PBYTE)Mem, 0, Params.Size);

		size_t i = 0;
		([&]
			{
				if (i >= Params.NameOffsetMap.size())
					return;

				auto& Param = Params.NameOffsetMap[i];

				if (((Param.PropertyFlags & 0x100) != 0 && (Param.PropertyFlags & 0x8000000) == 0) || (Param.PropertyFlags & 0x400) != 0)
				{
					i++;
					return;
				}

				const auto& Arg = args;

				memcpy(PBYTE(__int64(Mem) + Param.Offset), (const PBYTE)&Arg, Param.ElementSize);
				i++;
			}(), ...);

		ProcessEvent(Function, Mem);

		i = 0;
		([&] {
			if (i >= Params.NameOffsetMap.size())
				return;

			auto& Param = Params.NameOffsetMap[i];

			if (((Param.PropertyFlags & 0x100) == 0 && (Param.PropertyFlags & 0x8000000) == 0) || (Param.PropertyFlags & 0x400) != 0)
			{
				i++;
				return;
			}

			const auto& Arg = args;

			if constexpr (std::is_pointer_v<std::remove_reference_t<decltype(args)>>)
			{
				if (Arg != nullptr)
					memcpy((PBYTE)Arg, (const PBYTE)(__int64(Mem) + Param.Offset), Param.ElementSize);
			}
			else if constexpr (std::is_reference_v<decltype(args)>)
			{
				//if ((Param.PropertyFlags & 0x2) != 0)
				//	memcpy((PBYTE)&Arg, (const PBYTE)(__int64(Mem) + Param.Offset), Param.ElementSize);
			}
			i++;
			}(), ...);

		if constexpr (!std::is_void_v<Ret>)
		{
			Ret ret{};
			for (auto& Param : Params.NameOffsetMap)
			{
				if ((Param.PropertyFlags & 0x400) == 0)
					continue;

				memcpy((PBYTE)&ret, (const PBYTE)(__int64(Mem) + Param.Offset), Param.ElementSize);
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
		class UObject* Object;
		int32 Flags;
		int32 ClusterRootIndex;
		int32 SerialNumber;
	};

	class TUObjectArrayUnchunked
	{
	private:
		FUObjectItem* Objects;
		const int32 MaxElements;
		const int32 NumElements;

	public:
		inline int Num() const
		{
			return NumElements;
		}

		inline int Max() const
		{
			return MaxElements;
		}

		inline FUObjectItem* GetItemByIndex(const int32 Index) const
		{
			if (Index < 0 || Index > NumElements)
				return nullptr;

			return Objects + Index;
		}
	};

	class TUObjectArrayChunked
	{
	public:
		static inline auto NumElementsPerChunk = 0x10000;
	private:
		FUObjectItem** Objects;
		FUObjectItem* PreAllocatedObjects;
		const int32 MaxElements;
		const int32 NumElements;
		const int32 MaxChunks;
		const int32 NumChunks;

	public:
		inline int Num() const
		{
			return NumElements;
		}

		inline int Max() const
		{
			return MaxElements;
		}

		inline FUObjectItem* GetItemByIndex(const int32 Index) const
		{
			if (Index < 0 || Index > NumElements)
				return nullptr;

			const int32 ChunkIndex = Index / NumElementsPerChunk;
			const int32 ChunkOffset = Index % NumElementsPerChunk;

			return Objects[ChunkIndex] + ChunkOffset;
		}
	};

	class TUObjectArray
	{
	public:
		static const int32 Num()
		{
			auto GObjectsChunked = (TUObjectArrayChunked*&)Offsets::GObjectsChunked;
			auto GObjectsUnchunked = (TUObjectArrayUnchunked*&)Offsets::GObjectsUnchunked;
			return GObjectsChunked ? GObjectsChunked->Num() : GObjectsUnchunked->Num();
		}

		static const int32 Max()
		{
			auto GObjectsChunked = (TUObjectArrayChunked*&)Offsets::GObjectsChunked;
			auto GObjectsUnchunked = (TUObjectArrayUnchunked*&)Offsets::GObjectsUnchunked;
			return GObjectsChunked ? GObjectsChunked->Max() : GObjectsUnchunked->Max();
		}

		static FUObjectItem* GetItemByIndex(const int32 Index)
		{
			auto GObjectsChunked = (TUObjectArrayChunked*&)Offsets::GObjectsChunked;
			auto GObjectsUnchunked = (TUObjectArrayUnchunked*&)Offsets::GObjectsUnchunked;
			return GObjectsChunked ? GObjectsChunked->GetItemByIndex(Index) : GObjectsUnchunked->GetItemByIndex(Index);
		}

		static const UObject* GetObjectByIndex(const int32 Index)
		{
			const FUObjectItem* Item = GetItemByIndex(Index);
			return Item ? Item->Object : nullptr;
		}

		static const UObject* FindObject(const char* Name, uint64 TypeFlags = 0, const UClass* TargetClass = nullptr)
		{
			UEAllocatedString s = Name;
			UEAllocatedWString ws(s.begin(), s.end());
			auto ObjName = FName(ws);

			for (int i = 0; i < Num(); i++)
			{
				const UObject* Obj = GetObjectByIndex(i);
				if (Obj && Obj->Class && (TypeFlags == 0 || Obj->Class->GetCastFlags() & TypeFlags) && Obj->Name == ObjName && (!TargetClass || Obj->IsA(TargetClass)))
					return Obj;
			}
			return nullptr;
		}

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

		static const UObject* FindFirstObject(const char* Name)
		{
			UClass* TargetClass = (UClass*)FindObject(Name, 0x20);

			if (TargetClass)
				for (int i = 0; i < Num(); i++)
				{
					const UObject* Obj = GetObjectByIndex(i);
					if (Obj && !Obj->IsDefaultObject() && Obj->IsA(TargetClass))
						return Obj;
				}

			return nullptr;
		}
	};


	inline void UObject::AddToRoot() const
	{
		if (!this)
			return;

		auto Item = (FUObjectItem*)TUObjectArray::GetItemByIndex(Index);

		if (Item)
			Item->Flags |= 1 << 30;
	}

	inline const UClass* FindClass(const char* Name)
	{
		return (UClass*)TUObjectArray::FindObject(Name, 0x20);
	}

	inline const UObject* DefaultObjImpl(const char* Name)
	{
		auto TargetClass = FindClass(Name);
		for (int i = 0; i < TUObjectArray::Num(); i++)
		{
			const UObject* Obj = TUObjectArray::GetObjectByIndex(i);

			if (Obj && Obj->IsDefaultObject() && Obj->Class == TargetClass)
				return Obj;
		}
		return nullptr;
	}

	inline const UObject* DefaultObjImpl(const UClass* TargetClass, const char* Name)
	{
		for (int i = 0; i < TUObjectArray::Num(); i++)
		{
			const UObject* Obj = TUObjectArray::GetObjectByIndex(i);

			if (Obj && Obj->IsDefaultObject() && Obj->Class == TargetClass)
				return Obj;
		}
		return nullptr;
	}


	inline uint64_t UClass::GetCastFlags() const
	{
		static int32 Offset = 0;
		if (Offset == 0)
		{
			auto ClassObj = TUObjectArray::FindObject("Class");
			auto ActorObj = TUObjectArray::FindObject("Actor");
			for (int i = 0x28; i < 0x1a0; i += 4)
			{
				if (*(uint64_t*)(__int64(ClassObj) + i) == 0x29 && *(uint64_t*)(__int64(ActorObj) + i) == 0x1000000000)
				{
					Offset = i;
					break;
				}
			}
		}

		return *(uint64_t*)(__int64(this) + Offset);
	}

	inline UObject* UClass::GetDefaultObj() const
	{
		if (!this)
			return nullptr;

		static int32 Offset = 0;
		if (Offset == 0)
		{
			auto ClassClass = FindClass("Class");
			auto ActorClass = FindClass("Actor");
			auto ClassObj = DefaultObjImpl(ClassClass, "Class");
			auto ActorObj = DefaultObjImpl(ActorClass, "Actor");
			for (int i = 0x28; i < 0x1a0; i += 4)
			{
				if (*(UObject**)(__int64(ClassClass) + i) == ClassObj && *(UObject**)(__int64(ActorClass) + i) == ActorObj)
				{
					Offset = i;
					break;
				}
			}
		}

		return *(UObject**)(__int64(this) + Offset);
	}

	class UEnum : public UField
	{
	public:
		int64 GetValue(const char* EnumMemberName) const
		{
			if (!this)
				return -1;

			auto Names = *(TArray<TPair<FName, int64>>*)(__int64(this) + 0x40);

			for (int i = 0; i < Names.Num(); i++)
			{
				auto& Pair = Names[i];
				auto& Name = Pair.Key();
				auto& Value = Pair.Value();

				if (Name.ComparisonIndex)
				{
					auto str = Name.ToString();
					auto colcolIdx = str.find_last_of("::");

					auto RealName = colcolIdx == -1 ? str : str.substr(colcolIdx + 1);

					if (RealName == EnumMemberName)
						return Value;
				}
			}

			return -1;
		}
	};

	inline const UStruct* FindStruct(const char* Name)
	{
		return (UStruct*)TUObjectArray::FindObject(Name, 0x10);
	}

	inline const UEnum* FindEnum(const char* Name)
	{
		return (UEnum*)TUObjectArray::FindObject(Name, 0x4);
	}

	inline const UClass* UFunction::StaticClass()
	{
		static const SDK::UClass* _storage = nullptr;

		if (!_storage)
			_storage = SDK::FindClass("Function");

		return _storage;
	}


	inline const UClass* UClass::StaticClass()
	{
		static const SDK::UClass* _storage = nullptr;

		if (!_storage)
			_storage = SDK::FindClass("Class");

		return _storage;
	}

	inline const UClass* UObject::StaticClass()
	{
		static const SDK::UClass* _storage = nullptr;

		if (!_storage)
			_storage = SDK::FindClass("Object");

		return _storage;
	}

	inline int StartingSerial = 676767676; // scuffed
	class FWeakObjectPtr
	{
	public:
		int32                                         ObjectIndex;                                       // 0x0000(0x0004)(NOT AUTO-GENERATED PROPERTY)
		int32                                         ObjectSerialNumber;                                // 0x0004(0x0004)(NOT AUTO-GENERATED PROPERTY)


		FWeakObjectPtr(int32 Index = 0, int32 SerialNumber = 0)
			: ObjectIndex(Index), ObjectSerialNumber(SerialNumber)
		{
		}

		FWeakObjectPtr(const UObject* Object)
		{
			if (Object)
			{
				ObjectIndex = Object->Index;
				auto Item = TUObjectArray::GetItemByIndex(Object->Index);

				if (Item->SerialNumber == 0)
					Item->SerialNumber = StartingSerial++;

				ObjectSerialNumber = Item->SerialNumber;

			}
			else
			{
				ObjectIndex = -1;
				ObjectSerialNumber = 0;
			}
		}


	public:
		const UObject* Get() const
		{
			if (!this)
				return nullptr;

			if (ObjectIndex < 0 || ObjectSerialNumber == 0)
				return nullptr;

			auto Item = TUObjectArray::GetItemByIndex(ObjectIndex);

			if (!Item || Item->SerialNumber != ObjectSerialNumber)
				return nullptr;

			return Item->Object;
		}
		const UObject* operator->() const
		{
			return Get();
		}

		bool operator==(const FWeakObjectPtr& Other) const
		{
			return ObjectIndex == Other.ObjectIndex;
		}

		bool operator!=(const FWeakObjectPtr& Other) const
		{
			return ObjectIndex != Other.ObjectIndex;
		}

		bool operator==(const class UObject* Other) const
		{
			return ObjectIndex == Other->Index;
		}

		bool operator!=(const class UObject* Other) const
		{
			return ObjectIndex != Other->Index;
		}
	};

	template<typename UEType>
	class TWeakObjectPtr : public FWeakObjectPtr
	{
	public:
		TWeakObjectPtr(int32 Index = 0, int32 SerialNumber = 0)
			: FWeakObjectPtr(Index, SerialNumber)
		{
		}

		TWeakObjectPtr(UEType* Obj)
			: FWeakObjectPtr(Obj)
		{
		}

		UEType* Get() const
		{
			return (UEType*)FWeakObjectPtr::Get();
		}

		UEType* operator->() const
		{
			return (UEType*)FWeakObjectPtr::Get();
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
		const UObject* Get() const
		{
			return WeakPtr.Get();
		}
		const UObject* operator->() const
		{
			return WeakPtr.Get();
		}
	};

	struct FSoftObjectPath
	{
	public:
		class FName AssetPathName;
		class FString SubPathString;
	};


	__forceinline static const UObject* StaticFindObject(const wchar_t* ObjectPath, const UClass* Class)
	{
		auto StaticFindObjectInternal = (UObject * (*)(const UClass*, UObject*, const wchar_t*, bool)) SDK::Offsets::StaticFindObject;
		return StaticFindObjectInternal(Class, nullptr, ObjectPath, false);
	}

	__forceinline static const UObject* StaticLoadObject(const wchar_t* ObjectPath, const UClass* InClass, UObject* Outer = nullptr)
	{
		auto StaticLoadObjectInternal = (UObject * (*)(const UClass*, UObject*, const wchar_t*, const wchar_t*, uint32_t, UObject*, bool, void*)) SDK::Offsets::StaticLoadObject;
		return StaticLoadObjectInternal(InClass, Outer, ObjectPath, nullptr, 0, nullptr, false, nullptr);
	}

	static const UObject* FindObject(const wchar_t* ObjectPath, const UClass* Class)
	{
		auto Object = StaticFindObject(ObjectPath, Class);
		return Object ? Object : StaticLoadObject(ObjectPath, Class);
	}

	template <typename _Ot>
	static const _Ot* FindObject(const wchar_t* ObjectPath, const UClass* Class = _Ot::StaticClass())
	{
		return (const _Ot*)FindObject(ObjectPath, Class);
	}

	template <typename _Ot>
	static const _Ot* FindObject(UEAllocatedWString ObjectPath, const UClass* Class = _Ot::StaticClass())
	{
		return (const _Ot*)FindObject(ObjectPath.c_str(), Class);
	}

	template <typename _Ot>
	static const _Ot* FindObject(UEAllocatedString ObjectPath, const UClass* Class = _Ot::StaticClass())
	{
		return (const _Ot*)FindObject(std::wstring(ObjectPath.begin(), ObjectPath.end()).c_str(), Class);
	}

	template <typename _Ot>
	static const _Ot* FindObject(const char* ObjectPath, const UClass* Class = _Ot::StaticClass())
	{
		return FindObject<_Ot>(UEAllocatedString(ObjectPath), Class);
	}


	class FSoftObjectPtr : public TPersistentObjectPtr<FSoftObjectPath>
	{
	public:
		const UObject* InternalGet(const UClass* Class)
		{
			if (!this)
				return nullptr;

			auto Object = WeakPtr.Get();

			if (!Object)
			{
				const UObject* Ret = nullptr;

				if (VersionInfo.EngineVersion <= 4.16)
				{
					auto AssetLongPathname = *(FString*)(__int64(this) + offsetof(FSoftObjectPtr, ObjectID));

					if (AssetLongPathname.Num() > 0)
						WeakPtr = Ret = FindObject(AssetLongPathname.CStr(), Class);
				}
				else if (VersionInfo.FortniteVersion >= 23)
				{
					auto& PackageName = *(FName*)(__int64(this) + (VersionInfo.EngineVersion < 5.3 ? 0xC : 0x8));
					auto& AssetName = *(FName*)(__int64(this) + (VersionInfo.EngineVersion < 5.3 ? 0x10 : 0xC));
					auto& SubPathString = *(FString*)(__int64(this) + (VersionInfo.EngineVersion < 5.3 ? 0x14 : 0x10));

					if (PackageName.ComparisonIndex > 0)
					{
						auto FullPath = PackageName.ToWString();
						if (AssetName.ComparisonIndex > 0)
							FullPath += L"." + AssetName.ToWString();
						if (SubPathString.Num() > 0)
							FullPath += L":" + SubPathString.ToWString();

						WeakPtr = Ret = FindObject(FullPath.c_str(), Class);	
					}
				}
				else if (ObjectID.AssetPathName.ComparisonIndex > 0)
				{
					auto FullPath = ObjectID.AssetPathName.ToWString();
					if (ObjectID.SubPathString.Num() > 0)
						FullPath += L":" + ObjectID.SubPathString.ToWString();

					WeakPtr = Ret = FindObject(FullPath.c_str(), Class);
				}

				return Ret;
			}

			return Object;
		}

		static uint32_t Size()
		{
			return VersionInfo.EngineVersion >= 5.3 ? 0x20 : sizeof(FSoftObjectPtr);
		}
	};

	template<typename UEType>
	class TSoftObjectPtr : public FSoftObjectPtr
	{
	public:
		TSoftObjectPtr()
		{
		}

		TSoftObjectPtr(UEType* Obj)
		{
			WeakPtr = FWeakObjectPtr(Obj);
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
		TSoftClassPtr()
		{
		}

		TSoftClassPtr(UClass* Obj)
		{
			WeakPtr = FWeakObjectPtr(Obj);
			if (VersionInfo.FortniteVersion)
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
		const UObject* ObjectPointer = nullptr;
		const IInterface* InterfacePointer = nullptr;
	};

	template<class InterfaceType>
	class TScriptInterface : public FScriptInterface
	{
	};

	inline const IInterface* UObject::GetInterface(const UClass* Class) const
	{
		if (!Offsets::GetInterfaceAddress)
			return nullptr;

		return ((const IInterface * (*)(const UObject*, const UClass*)) Offsets::GetInterfaceAddress)(this, Class);
	}

	inline void UpdateNumElemsPerChunk()
	{
		TUObjectArrayChunked::NumElementsPerChunk = 0x10400;
	}

	inline void InitializeProcessEventVft(uintptr_t PEAddr)
	{
		auto DefaultObj = UObject::StaticClass()->GetDefaultObj();

		if (DefaultObj)
			for (int i = 0; i < 0x100; i++)
				if (__int64(DefaultObj->Vft[i]) == PEAddr)
				{
					Offsets::ProcessEventVft = i;
					break;
				}
	}
}