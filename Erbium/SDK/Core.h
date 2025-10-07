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

		bool IsValid() const 
		{
			return ComparisonIndex > 0;
		}

		UEAllocatedString ToString() const
		{
			if (!Offsets::AppendString)
			{
				if (IsBadReadPtr((void*) this))
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

	template<int32 StringLen>
	struct ConstexprString
	{
		char Chars[StringLen];

		consteval ConstexprString(const char(&Str)[StringLen])
		{
			std::copy_n(Str, StringLen, Chars);
		}

		operator const char* () const
		{
			return static_cast<const char*>(Chars);
		}
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

		uint32 GetOffset(const char* Name) const 
		{
			static auto OffsetOff = VersionInfo.EngineVersion >= 4.25 && VersionInfo.FortniteVersion < 20 ? 0x4c : (VersionInfo.EngineVersion >= 5.2 ? 0x3c : 0x44);
			auto Prop = GetProperty(Name);
			if (!Prop) return -1;
			return GetFromOffset<uint32>(Prop, OffsetOff);
		}

		template <ConstexprString Name, typename T = UObject*>
		T& Get() const 
		{
			auto Off = GetOffset(Name);
			if (Off == -1) {
				printf("Failed to get offset for %s!\n", Name.Chars);
				while (true);
			}
			return GetFromOffset<T>(this, Off);
		}

		template <ConstexprString Name>
		bool GetBitfield() const
		{
			static auto OffsetOff = VersionInfo.EngineVersion >= 4.25 && VersionInfo.FortniteVersion < 20 ? 0x4c : (VersionInfo.EngineVersion >= 5.2 ? 0x3c : 0x44);
			auto Prop = GetProperty(Name);
			auto Offset = GetFromOffset<uint32>(Prop, OffsetOff);
			auto& Field = GetFromOffset<uint8>(this, Offset);

			return Field & Prop->GetFieldMask();
		}

		template <ConstexprString Name>
		void SetBitfield(bool Value) const 
		{
			static auto OffsetOff = VersionInfo.EngineVersion >= 4.25 && VersionInfo.FortniteVersion < 20 ? 0x4c : (VersionInfo.EngineVersion >= 5.2 ? 0x3c : 0x44);
			auto Prop = GetProperty(Name);
			auto Offset = GetFromOffset<uint32>(Prop, OffsetOff);
			auto& Field = GetFromOffset<uint8>(this, Offset);

			if (Value) {
				Field |= Prop->GetFieldMask();
			}
			else {
				Field &= ~Prop->GetFieldMask();
			}
		}

		template <ConstexprString Name>
		bool Has() const 
		{
			return GetOffset(Name) != -1;
		}

		template <ConstexprString Name, typename T = UObject*>
		T* GetPtr() const 
		{
			auto Off = GetOffset(Name);

			if (Off == -1) 
				return nullptr;

			return GetPtrFromOffset<T>(this, Off);
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

			for (auto _Clss = Class; _Clss; _Clss = GetFromOffset<UClass*>(_Clss, 0x30)) {
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

		class UFunction* GetFunction(const char* Name) const
		{
			return (UFunction*)GetProperty(Name, 0x80000);
		}

		void ProcessEvent(class UFunction* Function, void* Params) const 
		{
			auto ProcessEventInternal = (void(*&)(const UObject*, class UFunction*, void*)) Offsets::ProcessEvent;
			ProcessEventInternal(this, Function, Params);
		}

		template <ConstexprString Name>
		void Call(void* Params = nullptr) const 
		{
			auto Function = GetFunction(Name);
			ProcessEvent(Function, Params);
		}

		template <ConstexprString Name>
		void Call(UEAllocatedVector<ParamPair> Params) const;

		template <typename Ret = void, typename... Args>
		Ret Call(UFunction* Function, Args... args) const;

		static const UClass* StaticClass();

		const class IInterface* GetInterface(const class UClass*) const;
		
		void AddToRoot() const;
	};

	class UField : public UObject
	{
	public:
		const UField* GetNext(bool bNewFields = false) const 
		{
			auto NextOff = bNewFields ? (VersionInfo.EngineVersion >= 5.2 ? 0x18 : 0x20) : 0x28;

			return GetFromOffset<UField*>(this, NextOff);
		}

		FName& GetName(bool bNewFields = false) const
		{
			auto NameOff = bNewFields ? (VersionInfo.EngineVersion >= 5.2 ? 0x20 : 0x28) : 0x18;

			return GetFromOffset<FName>(this, NameOff);
		}

		const uint8 GetFieldMask() const 
		{
			static auto FieldMaskOff = VersionInfo.EngineVersion >= 4.25 && VersionInfo.FortniteVersion < 20 ? 0x7b : 0x73;

			return GetFromOffset<uint8>(this, FieldMaskOff);
		}
	};

	class UStruct : public UField 
	{
	public:
		const UStruct* GetSuper() const 
		{
			static auto SuperOff = VersionInfo.EngineVersion >= 4.22 ? 0x40 : 0x30;

			return GetFromOffset<UStruct*>(this, SuperOff);
		}

		const int32 GetPropertiesSize() const 
		{
			auto ChildrenOff = VersionInfo.EngineVersion >= 4.25 ? 0x58 : (VersionInfo.EngineVersion >= 4.22 ? 0x50 : 0x40);

			return GetFromOffset<int32>(this, ChildrenOff);
		}

		const UField* GetChildren(bool bNewFields = false) const 
		{
			auto ChildrenOff = bNewFields ? 0x50 : (VersionInfo.EngineVersion >= 4.22 ? 0x48 : 0x38);

			return GetFromOffset<UField*>(this, ChildrenOff);
		}


		const UField* GetProperty(const char* Name, uint64_t CastFlags = 0) const;

		uint32_t GetOffset(const char* Name) const
		{
			static auto OffsetOff = VersionInfo.EngineVersion >= 4.25 && VersionInfo.FortniteVersion < 20 ? 0x4c : (VersionInfo.EngineVersion >= 5.2 ? 0x3c : 0x44);
			auto Prop = GetProperty(Name);
			if (!Prop)
				return -1;

			return GetFromOffset<uint32>(Prop, OffsetOff);
		}
	};

	class UClass : public UStruct 
	{
	public:
		uint64_t GetCastFlags() const;
		static const UClass* StaticClass();

		UObject* GetDefaultObj() const;
	};

	inline const UField* UStruct::GetProperty(const char* Name, uint64_t CastFlags) const
	{
		for (const UStruct* Clss = this; Clss; Clss = (const UStruct*)Clss->GetSuper())
		{
			if (CastFlags != 0x80000 && VersionInfo.EngineVersion >= 4.25) {
				for (const UField* Prop = Clss->GetChildren(true); Prop; Prop = Prop->GetNext(true))
				{
					if (Prop->GetName(true).ToSDKString() == Name)
						return Prop;
				}
			}

			for (const UField* Prop = Clss->GetChildren(false); Prop; Prop = Prop->GetNext(false))
			{
				if ((CastFlags == 0 || Prop->Class->GetCastFlags() & CastFlags) && Prop->GetName(false).ToSDKString() == Name)
					return Prop;
			}
		}

		return nullptr;
	}

	inline const UField* UObject::GetProperty(const char* Name, uint64_t CastFlags) const
	{
		return Class->GetProperty(Name, CastFlags);
	}

	class UFunction : public UStruct 
	{
	public:
		void*& GetNativeFunc()
		{
			if (VersionInfo.FortniteVersion <= 6.31)
				return GetFromOffset<void*>(this, 0xB0);
			else if (VersionInfo.FortniteVersion > 7.00 && VersionInfo.FortniteVersion < 12.00)
				return GetFromOffset<void*>(this, 0xC0);
			else if (VersionInfo.FortniteVersion >= 12.00 && VersionInfo.FortniteVersion < 12.10)
				return GetFromOffset<void*>(this, 0xC8);
			else if (VersionInfo.FortniteVersion >= 12.10 && VersionInfo.FortniteVersion <= 12.61)
				return GetFromOffset<void*>(this, 0xF0);
			else
				return GetFromOffset<void*>(this, 0xD8);
		}

		void SetNativeFunc(void* NewFunc)
		{
			GetNativeFunc() = NewFunc;
		}

		__declspec(property(get = GetNativeFunc, put = SetNativeFunc))
		void* ExecFunction;

		void Call(const UObject* obj, void* Params) 
		{
			if (this) 
				obj->ProcessEvent(this, Params);
		}

		void Call(const UObject* obj, UEAllocatedVector<ParamPair> Params) 
		{
			if (this) 
				obj->ProcessEvent(this, CreateParams(Params));
		}

		void operator()(const UObject* obj, void* Params) 
		{
			return Call(obj, Params);
		}

		uint32 GetVTableIndex() 
		{
			if (!this) 
				return -1;

			auto ValidateName = Name.ToString() + "_Validate";
			auto ValidateRef = Memcury::Scanner::FindStringRef(UEAllocatedWString(ValidateName.begin(), ValidateName.end()).c_str(), false);

			auto Addr = ValidateRef.Get();

			if (!Addr)
				Addr = __int64(GetNativeFunc());

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
			UEAllocatedString Name;
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

		Params GetParams() 
		{
			Params p{};
			static auto OffsetOff = VersionInfo.EngineVersion >= 4.25 && VersionInfo.FortniteVersion < 20 ? 0x4c : (VersionInfo.EngineVersion >= 5.2 ? 0x3c : 0x44);
			static auto PropertyFlagsOff = OffsetOff - 0xc;
			static auto ElementSizeOff = OffsetOff - 0x10;

			if (VersionInfo.EngineVersion >= 4.25)
				for (const UField* _Pr = GetChildren(true); _Pr; _Pr = _Pr->GetNext(true))
					p.NameOffsetMap.push_back({ _Pr->GetName(true).ToSDKString(), GetFromOffset<uint32>(_Pr, OffsetOff), GetFromOffset<uint64>(_Pr, PropertyFlagsOff), GetFromOffset<uint32>(_Pr, ElementSizeOff) });

			for (const UField* _Pr = GetChildren(false); _Pr; _Pr = _Pr->GetNext(false))
				p.NameOffsetMap.push_back({ _Pr->GetName(false).ToSDKString(), GetFromOffset<uint32>(_Pr, OffsetOff), GetFromOffset<uint64>(_Pr, PropertyFlagsOff), GetFromOffset<uint32>(_Pr, ElementSizeOff) });

			p.Size = GetPropertiesSize();
			return p;
		}

		void* CreateParams(UEAllocatedVector<ParamPair> InputParams) 
		{
			auto Params = GetParams();
			auto Mem = FMemory::Malloc(Params.Size);
			__stosb((PBYTE) Mem, 0, Params.Size);

			for (auto& InputParam : InputParams) 
			{
				Param FoundParam;
				int i = 0;
				uint32 Size = 0;
				for (auto& FuncParam : Params.NameOffsetMap) 
				{
					if (FuncParam.Name == InputParam.ParamName) 
					{
						FoundParam = FuncParam;
						Size = i == Params.NameOffsetMap.size() - 1 ? Params.Size - FuncParam.Offset : Params.NameOffsetMap[i + 1].Offset - FuncParam.Offset;
						break;
					}
					i++;
				}

				if (Size)
					memcpy((PBYTE)Mem + FoundParam.Offset, InputParam.Value, Size);
			}

			return Mem;
		}

		template <typename _Rt>
		_Rt* GetValueFromParams(void* Params, const char* Name) 
		{
			auto Params = GetParams();

			for (auto& FuncParam : Params.NameOffsetMap) {
				if (FuncParam.Name == Name) {
					return GetPtrFromOffset<_Rt>(Params, FuncParam.Offset);
				}
			}

			return nullptr;
		}
	};

	template <ConstexprString Name>
	void UObject::Call(UEAllocatedVector<ParamPair> Params) const 
	{
		auto Function = GetFunction(Name);
		ProcessEvent(Function, Function->CreateParams(Params));
	}

	template <typename Ret, typename... Args>
	Ret UObject::Call(UFunction* Function, Args... args) const
	{

		if (!Function)
			return Ret();

		if constexpr (sizeof...(args) == 0 && std::is_void_v<Ret>)
			return ProcessEvent(Function, nullptr);

		auto Params = Function->GetParams();
		auto Mem = FMemory::Malloc(Params.Size);
		__stosb((PBYTE)Mem, 0, Params.Size);

		size_t i = 0;
		([&] 
		{
			if (i >= Params.NameOffsetMap.size())
				return;

			auto& Param = Params.NameOffsetMap[i];

			if ((Param.PropertyFlags & 0x100) != 0 && (Param.PropertyFlags & 0x8000000) == 0)
			{
				i++;
				return;
			}

			const auto& Arg = args;

			__movsb(PBYTE(__int64(Mem) + Param.Offset), (const PBYTE)&Arg, Param.ElementSize);
			i++;
		}(), ...);

		ProcessEvent(Function, Mem);

		i = 0;
		([&] {
			if (i >= Params.NameOffsetMap.size())
				return;

			auto& Param = Params.NameOffsetMap[i];

			if ((Param.PropertyFlags & 0x100) == 0 && (Param.PropertyFlags & 0x8000000) == 0)
			{
				i++;
				return;
			}

			const auto& Arg = args;

			if constexpr (std::is_pointer_v<decltype(args)>)
			{
				if (Arg != nullptr)
					__movsb((PBYTE)Arg, (const PBYTE)(__int64(Mem) + Param.Offset), Param.ElementSize);
			}
			else if constexpr (std::is_reference_v<decltype(args)>)
			{
				if ((Param.PropertyFlags & 0x2) != 0)
					__movsb((PBYTE)&Arg, (const PBYTE)(__int64(Mem) + Param.Offset), Param.ElementSize);
			}
			i++;
		}(), ...);

		if constexpr (!std::is_void_v<Ret>)
		{
			Ret ret{};
			int x = 0;
			for (auto& Param : Params.NameOffsetMap)
			{
				if (Param.Name == "ReturnValue")
				{
					__movsb((PBYTE)&ret, (const PBYTE)(__int64(Mem) + Param.Offset), Param.ElementSize);
					break;
				}
				x++;
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
		const FUObjectItem* Objects;
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

		inline const FUObjectItem* GetItemByIndex(const int32 Index) const
		{
			if (Index < 0 || Index > NumElements)
				return nullptr;

			return Objects + Index;
		}
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
		inline int Num() const
		{
			return NumElements;
		}

		inline int Max() const
		{
			return MaxElements;
		}

		inline const FUObjectItem* GetItemByIndex(const int32 Index) const
		{
			if (Index < 0 || Index > NumElements)
				return nullptr;

			const int32 ChunkIndex = Index / 0x10000;
			const int32 ChunkOffset = Index % 0x10000;

			return Objects[ChunkIndex] + ChunkOffset;
		}
	};

	class TUObjectArray {
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

		static const FUObjectItem* GetItemByIndex(const int32 Index) 
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
			for (int i = 0; i < Num(); i++) {
				const UObject* Obj = GetObjectByIndex(i);
				if (Obj && Obj->Class && (TypeFlags == 0 || Obj->Class->GetCastFlags() & TypeFlags) && (!TargetClass || Obj->IsA(TargetClass)) && Obj->Name.ToString() == Name)
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
			for (int i = 0; i < Num(); i++) {
				const UObject* Obj = GetObjectByIndex(i);
				if (Obj && !Obj->IsDefaultObject() && Obj->IsA(TargetClass))
					return Obj;
			}
			return nullptr;
		}
	};


	inline void UObject::AddToRoot() const
	{
		auto Item = (FUObjectItem*)TUObjectArray::GetItemByIndex(Index);

		if (Item)
		{
			Item->Flags |= 1 << 30;
		}
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
		static int32 Offset = 0;
		if (Offset == 0)
		{
			auto ClassClass = TUObjectArray::FindObject("Class");
			auto ActorClass = TUObjectArray::FindObject("Actor");
			auto ClassObj = TUObjectArray::FindObject("Default__Class");
			auto ActorObj = TUObjectArray::FindObject("Default__Actor");
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
		int64 GetValue(const char *EnumMemberName) const
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
					if (Name.ToString().contains(EnumMemberName))
						return Value;
				}
			}

			return -1;
		}
	};

	inline const UClass* FindClass(const char* Name) {
		return (UClass*)TUObjectArray::FindObject(Name, 0x20);
	}

	inline const UStruct* FindStruct(const char* Name) {
		return (UStruct*)TUObjectArray::FindObject(Name, 0x10);
	}

	inline const UEnum* FindEnum(const char* Name) {
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

	inline const UObject* DefaultObjImpl(const char* Name) {
		auto TargetClass = FindClass(Name);
		for (int i = 0; i < TUObjectArray::Num(); i++) {
			const UObject* Obj = TUObjectArray::GetObjectByIndex(i);
			if (Obj && Obj->IsDefaultObject() && Obj->IsA(TargetClass))
				return Obj;
		}
		return nullptr;
	}
	template <typename T = UObject*, typename _St>
	inline T& StructGet(_St* StructInstance, const char* StructName, const char* Name) {
		auto Struct = TUObjectArray::FindObject<UStruct>(StructName, 0x10);

		static auto OffsetOff = VersionInfo.EngineVersion >= 4.25 && VersionInfo.FortniteVersion < 20 ? 0x4c : (VersionInfo.EngineVersion >= 5.2 ? 0x3c : 0x44);
		auto Off = GetFromOffset<uint32>(Struct->GetProperty(Name), OffsetOff);
		return GetFromOffset<T>(StructInstance, Off);
	}

	template <typename _St>
	inline bool StructHas(_St* StructInstance, const char* StructName, const char* Name) {
		auto Struct = TUObjectArray::FindObject<UStruct>(StructName, 0x10);

		return Struct->GetProperty(Name) != nullptr;
	}

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
				ObjectSerialNumber = TUObjectArray::GetItemByIndex(Object->Index)->SerialNumber;
			}
		}


	public:
		const UObject* Get() const
		{
			return TUObjectArray::GetObjectByIndex(ObjectIndex);
		}
		const UObject* operator->() const
		{
			return TUObjectArray::GetObjectByIndex(ObjectIndex);
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
			return static_cast<UEType*>(FWeakObjectPtr::Get());
		}

		UEType* operator->() const
		{
			return static_cast<UEType*>(FWeakObjectPtr::Get());
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
		auto StaticLoadObjectInternal = (UObject * (*)(const UClass*, UObject*, const wchar_t*, const wchar_t*, uint32_t, UObject*, bool)) SDK::Offsets::StaticLoadObject;
		return StaticLoadObjectInternal(InClass, Outer, ObjectPath, nullptr, 0, nullptr, false);
	}

	static const UObject* FindObject(const wchar_t* ObjectPath, const UClass* Class)
	{
		auto Object = StaticFindObject(ObjectPath, Class);
		return Object ? Object : StaticLoadObject(ObjectPath, Class);
	}

	class FSoftObjectPtr : public TPersistentObjectPtr<FSoftObjectPath>
	{
	public:
		const UObject* InternalGet(const UClass* Class)
		{
			if (!this)
				return nullptr;

			auto Ret = WeakPtr.ObjectIndex && WeakPtr.ObjectSerialNumber ? Get() : nullptr;
			if (VersionInfo.EngineVersion <= 4.16)
			{
				auto AssetLongPathname = *(FString*)(__int64(this) + offsetof(FSoftObjectPtr, ObjectID));

				if ((!Ret || !Ret->IsA(Class)) && AssetLongPathname.Num() > 0)
					WeakPtr = Ret = FindObject(AssetLongPathname.CStr(), Class);
			}
			else if ((!Ret || !Ret->IsA(Class)) && ObjectID.AssetPathName.ComparisonIndex > 0)
				WeakPtr = Ret = FindObject(ObjectID.AssetPathName.ToWString().c_str(), Class);
			return Ret;
		}
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

	inline const IInterface* UObject::GetInterface(const UClass* Class) const
	{
		if (!Offsets::GetInterfaceAddress)
			return nullptr;

		return ((const IInterface * (*)(const UObject*, const UClass*)) Offsets::GetInterfaceAddress)(this, Class);
	}
}