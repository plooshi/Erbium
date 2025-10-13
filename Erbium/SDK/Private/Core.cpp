#include "pch.h"

bool FName::IsValid() const
{
	return ComparisonIndex > 0;
}

UEAllocatedString FName::ToString() const
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

UEAllocatedString FName::ToSDKString() const
{
	UEAllocatedString OutputString = ToString();

	size_t pos = OutputString.rfind('/');

	if (pos == UEAllocatedString::npos)
		return OutputString;

	return OutputString.substr(pos + 1);
}

UEAllocatedWString FName::ToWString() const
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

UEAllocatedWString FName::ToSDKWString() const
{
	UEAllocatedWString OutputString = ToWString();

	size_t pos = OutputString.rfind('/');

	if (pos == UEAllocatedWString::npos)
		return OutputString;

	return OutputString.substr(pos + 1);
}

bool FName::operator==(const FName& Other) const
{
	return ComparisonIndex == Other.ComparisonIndex && (VersionInfo.FortniteVersion >= 20.00 || Number == Other.Number);
}

bool FName::operator!=(const FName& Other) const
{
	return ComparisonIndex != Other.ComparisonIndex || (VersionInfo.FortniteVersion >= 20.00 ? false : Number != Other.Number);
}

bool FName::operator<(const FName& Other) const
{
	return ComparisonIndex == Other.ComparisonIndex ? (VersionInfo.FortniteVersion < 20.00 && Number < Other.Number) : ComparisonIndex < Other.ComparisonIndex;
}

FName::operator bool() const {
	return IsValid();
}

const UField* UObject::GetProperty(const char* Name, uint64_t CastFlags) const
{
	return Class->GetProperty(Name, CastFlags);
}

bool UObject::IsDefaultObject() const
{
	return ObjectFlags & 0x10;
}

uint32 UObject::GetOffset(const char* Name, uint64_t CastFlags) const
{
	auto Prop = GetProperty(Name, CastFlags);

	if (!Prop) 
		return -1;

	return GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);
}

bool UObject::IsA(const class UClass* Clss) const
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
		if (_Clss == Clss) return true;

	return false;
}

__declspec(noinline) UFunction* UObject::GetFunction(const char* Name) const
{
	UEAllocatedString s = Name;

	for (const UStruct* Clss = Class; Clss; Clss = (const UStruct*)Clss->GetSuper())
		for (const UField* Prop = Clss->GetChildren(); Prop; Prop = Prop->GetNext())
			if (Prop->Class->GetCastFlags() & 0x80000 && Prop->GetName().ToSDKString() == s)
				return (UFunction*)Prop;

	return nullptr;
	//return (UFunction*)GetProperty(Name, 0x80000);
}

void UObject::ProcessEvent(class UFunction* Function, void* Params) const
{
	auto& ProcessEventInternal = (void(*&)(const UObject*, class UFunction*, void*)) Offsets::ProcessEvent;
	ProcessEventInternal(this, Function, Params);
}

const UClass* UObject::StaticClass()
{
	static const SDK::UClass* _storage = nullptr;

	if (!_storage)
		_storage = SDK::FindClass("Object");

	return _storage;
}

const IInterface* UObject::GetInterface(const UClass* Class) const
{
	if (!Offsets::GetInterfaceAddress)
		return nullptr;

	return ((const IInterface * (*&)(const UObject*, const UClass*)) Offsets::GetInterfaceAddress)(this, Class);
}

void UObject::AddToRoot() const
{
	auto Item = (FUObjectItem*)TUObjectArray::GetItemByIndex(Index);

	if (Item)
	{
		Item->Flags |= 1 << 30;
	}
}

const UField* UField::FField_GetNext() const
{
	return GetFromOffset<UField*>(this, Offsets::FField_Next);
}

FName& UField::FField_GetName() const
{
	return GetFromOffset<FName>(this, Offsets::FField_Name);
}

const UField* UField::GetNext() const
{
	return GetFromOffset<UField*>(this, 0x28);
}

FName& UField::GetName() const
{
	return GetFromOffset<FName>(this, 0x18);
}

const uint8 UField::GetFieldMask() const
{
	return GetFromOffset<uint8>(this, Offsets::FieldMask);
}

const UStruct* UStruct::GetSuper() const
{
	return GetFromOffset<UStruct*>(this, Offsets::Super);
}

const int32 UStruct::GetPropertiesSize() const
{
	return GetFromOffset<int32>(this, Offsets::PropertiesSize);
}

const UField* UStruct::GetChildProperties() const
{
	return GetFromOffset<UField*>(this, 0x50);
}

const UField* UStruct::GetChildren() const
{
	return GetFromOffset<UField*>(this, Offsets::Children);
}

__declspec(noinline) const UField* UStruct::GetProperty(const char* Name, uint64_t CastFlags) const
{
	UEAllocatedString s = Name;

	for (const UStruct* Clss = this; Clss; Clss = (const UStruct*)Clss->GetSuper())
	{
		if (VersionInfo.EngineVersion >= 4.25)
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
				if (Prop->FField_GetName().ToSDKString() == s)
					return Prop;
			}
		}
		else
		{
			for (const UField* Prop = Clss->GetChildren(); Prop; Prop = Prop->GetNext())
			{
				if ((CastFlags == 0 || Prop->Class->GetCastFlags() & CastFlags) && Prop->GetName().ToSDKString() == s)
					return Prop;
			}
		}
	}

	return nullptr;
}

uint32_t UStruct::GetOffset(const char* Name, uint64_t CastFlags) const
{
	auto Prop = GetProperty(Name, CastFlags);

	if (!Prop)
		return -1;

	return GetFromOffset<uint32>(Prop, Offsets::Offset_Internal);
}

uint64_t UClass::GetCastFlags() const
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

const UClass* UClass::StaticClass()
{
	static const SDK::UClass* _storage = nullptr;

	if (!_storage)
		_storage = SDK::FindClass("Class");

	return _storage;
}

UObject* UClass::GetDefaultObj() const
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

void*& UFunction::GetNativeFunc()
{
	return GetFromOffset<void*>(this, Offsets::ExecFunction);
}

void UFunction::SetNativeFunc(void* NewFunc)
{
	GetNativeFunc() = NewFunc;
}


void* UFunction::GetImpl()
{
	if (!this)
		return nullptr;

	return (void*)Memcury::Scanner(GetNativeFunc()).ScanFor({ 0x40, 0x0F, 0x95, 0xC7 }).ScanFor({ 0xE8 }).RelativeOffset(1).Get();

}

void UFunction::Call(const UObject* obj, void* Params)
{
	if (this)
		obj->ProcessEvent(this, Params);
}

void UFunction::operator()(const UObject* obj, void* Params)
{
	return Call(obj, Params);
}

uint32 UFunction::GetVTableIndex()
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

const UClass* UFunction::StaticClass()
{
	static const SDK::UClass* _storage = nullptr;

	if (!_storage)
		_storage = SDK::FindClass("Function");

	return _storage;
}

UEAllocatedVector<UFunction::Param> UFunction::GetParams()
{
	UEAllocatedVector<UFunction::Param> p{};

	if (VersionInfo.EngineVersion >= 4.25)
		for (const UField* _Pr = GetChildProperties(); _Pr; _Pr = _Pr->FField_GetNext())
			p.push_back({ /*_Pr->GetName(true).ToSDKString(), */GetFromOffset<uint32>(_Pr, Offsets::Offset_Internal), GetFromOffset<uint64>(_Pr, Offsets::PropertyFlags), GetFromOffset<uint32>(_Pr, Offsets::ElementSize) });
	else
		for (const UField* _Pr = GetChildren(); _Pr; _Pr = _Pr->GetNext())
			p.push_back({ /*_Pr->GetName(false).ToSDKString(), */GetFromOffset<uint32>(_Pr, Offsets::Offset_Internal), GetFromOffset<uint64>(_Pr, Offsets::PropertyFlags), GetFromOffset<uint32>(_Pr, Offsets::ElementSize) });

	return p;
}


UEAllocatedVector<UFunction::ParamNamed> UFunction::GetParamsNamed()
{
	UEAllocatedVector<UFunction::ParamNamed> p{};

	if (VersionInfo.EngineVersion >= 4.25)
		for (const UField* _Pr = GetChildProperties(); _Pr; _Pr = _Pr->FField_GetNext())
			p.push_back({ GetFromOffset<uint32>(_Pr, Offsets::Offset_Internal), GetFromOffset<uint64>(_Pr, Offsets::PropertyFlags), GetFromOffset<uint32>(_Pr, Offsets::ElementSize), _Pr->FField_GetName().ToSDKString() });
	else
		for (const UField* _Pr = GetChildren(); _Pr; _Pr = _Pr->GetNext())
			p.push_back({ GetFromOffset<uint32>(_Pr, Offsets::Offset_Internal), GetFromOffset<uint64>(_Pr, Offsets::PropertyFlags), GetFromOffset<uint32>(_Pr, Offsets::ElementSize), _Pr->GetName().ToSDKString() });

	return p;
}

inline int TUObjectArrayUnchunked::Num() const
{
	return NumElements;
}

inline int TUObjectArrayUnchunked::Max() const
{
	return MaxElements;
}

inline const FUObjectItem* TUObjectArrayUnchunked::GetItemByIndex(const int32 Index) const
{
	if (Index < 0 || Index > NumElements)
		return nullptr;

	return Objects + Index;
}

const UObject* TUObjectArrayUnchunked::GetObjectByIndex(const int32 Index)
{
	const FUObjectItem* Item = GetItemByIndex(Index);
	return Item ? Item->Object : nullptr;
}

const UObject* TUObjectArrayUnchunked::FindObject(const char* Name, uint64 TypeFlags, const UClass* TargetClass)
{
	UEAllocatedString s = Name;

	for (int i = 0; i < Num(); i++) {
		const UObject* Obj = GetObjectByIndex(i);
		if (Obj && Obj->Class && (TypeFlags == 0 || Obj->Class->GetCastFlags() & TypeFlags) && (!TargetClass || Obj->IsA(TargetClass)) && Obj->Name.ToString() == s)
			return Obj;
	}
	return nullptr;
}

inline int TUObjectArrayChunked::Num() const
{
	return NumElements;
}

inline int TUObjectArrayChunked::Max() const
{
	return MaxElements;
}

inline const FUObjectItem* TUObjectArrayChunked::GetItemByIndex(const int32 Index) const
{
	if (Index < 0 || Index > NumElements)
		return nullptr;

	const int32 ChunkIndex = Index / 0x10000;
	const int32 ChunkOffset = Index % 0x10000;

	return Objects[ChunkIndex] + ChunkOffset;
}

const UObject* TUObjectArrayChunked::GetObjectByIndex(const int32 Index)
{
	const FUObjectItem* Item = GetItemByIndex(Index);
	return Item ? Item->Object : nullptr;
}

__declspec(noinline) const UObject* TUObjectArrayChunked::FindObject(const char* Name, uint64 TypeFlags, const UClass* TargetClass)
{
	UEAllocatedString s = Name;
	for (int i = 0; i < Num(); i++) {
		const UObject* Obj = GetObjectByIndex(i);
		if (Obj && Obj->Class && (TypeFlags == 0 || Obj->Class->GetCastFlags() & TypeFlags) && (!TargetClass || Obj->IsA(TargetClass)) && Obj->Name.ToString() == s)
			return Obj;
	}
	return nullptr;
}

const int32 TUObjectArray::Num()
{
	auto GObjectsChunked = (TUObjectArrayChunked*&)Offsets::GObjectsChunked;
	auto GObjectsUnchunked = (TUObjectArrayUnchunked*&)Offsets::GObjectsUnchunked;

	return GObjectsChunked ? GObjectsChunked->Num() : GObjectsUnchunked->Num();
}

const int32 TUObjectArray::Max()
{
	auto GObjectsChunked = (TUObjectArrayChunked*&)Offsets::GObjectsChunked;
	auto GObjectsUnchunked = (TUObjectArrayUnchunked*&)Offsets::GObjectsUnchunked;

	return GObjectsChunked ? GObjectsChunked->Max() : GObjectsUnchunked->Max();
}

const FUObjectItem* TUObjectArray::GetItemByIndex(const int32 Index)
{
	auto GObjectsChunked = (TUObjectArrayChunked*&)Offsets::GObjectsChunked;
	auto GObjectsUnchunked = (TUObjectArrayUnchunked*&)Offsets::GObjectsUnchunked;

	return GObjectsChunked ? GObjectsChunked->GetItemByIndex(Index) : GObjectsUnchunked->GetItemByIndex(Index);
}

const UObject* TUObjectArray::GetObjectByIndex(const int32 Index)
{
	const FUObjectItem* Item = GetItemByIndex(Index);

	return Item ? Item->Object : nullptr;
}

const UObject* TUObjectArray::FindObject(const char* Name, uint64 TypeFlags, const UClass* TargetClass)
{
	auto GObjectsChunked = (TUObjectArrayChunked*&)Offsets::GObjectsChunked;
	auto GObjectsUnchunked = (TUObjectArrayUnchunked*&)Offsets::GObjectsUnchunked;

	return GObjectsChunked ? GObjectsChunked->FindObject(Name, TypeFlags, TargetClass) : GObjectsUnchunked->FindObject(Name, TypeFlags, TargetClass);
}

const UObject* TUObjectArray::FindFirstObject(const char* Name)
{
	UClass* TargetClass = (UClass*)FindObject(Name, 0x20);
	for (int i = 0; i < Num(); i++) {
		const UObject* Obj = GetObjectByIndex(i);
		if (Obj && !Obj->IsDefaultObject() && Obj->IsA(TargetClass))
			return Obj;
	}
	return nullptr;
}

const UClass* SDK::FindClass(const char* Name) 
{
	return (UClass*)TUObjectArray::FindObject(Name, 0x20);
}

const UObject* SDK::DefaultObjImpl(const char* Name)
{
	auto TargetClass = FindClass(Name);
	for (int i = 0; i < TUObjectArray::Num(); i++) {
		const UObject* Obj = TUObjectArray::GetObjectByIndex(i);
		if (Obj && Obj->IsDefaultObject() && Obj->Class == TargetClass)
			return Obj;
	}
	return nullptr;
}

const UObject* SDK::DefaultObjImpl(const UClass* TargetClass, const char* Name)
{
	for (int i = 0; i < TUObjectArray::Num(); i++) {
		const UObject* Obj = TUObjectArray::GetObjectByIndex(i);
		if (Obj && Obj->IsDefaultObject() && Obj->Class == TargetClass)
			return Obj;
	}
	return nullptr;
}

const UStruct* SDK::FindStruct(const char* Name)
{
	return (UStruct*)TUObjectArray::FindObject(Name, 0x10);
}

const UEnum* SDK::FindEnum(const char* Name)
{
	return (UEnum*)TUObjectArray::FindObject(Name, 0x4);
}

int64 UEnum::GetValue(const char* Member) const
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
			if (Name.ToString() == Member)
				return Value;
		}
	}

	return -1;
}

FWeakObjectPtr::FWeakObjectPtr(const UObject* Object)
{
	if (Object)
	{
		ObjectIndex = Object->Index;
		ObjectSerialNumber = TUObjectArray::GetItemByIndex(Object->Index)->SerialNumber;
	}
}

FWeakObjectPtr::FWeakObjectPtr(UObject* Object)
{
	if (Object)
	{
		ObjectIndex = Object->Index;
		ObjectSerialNumber = TUObjectArray::GetItemByIndex(Object->Index)->SerialNumber;
	}
}

const UObject* FWeakObjectPtr::Get() const
{
	if (ObjectIndex < 0 || ObjectSerialNumber == 0)
		return nullptr;

	auto Item = TUObjectArray::GetItemByIndex(ObjectIndex);

	if (!Item || Item->SerialNumber != ObjectSerialNumber)
		return nullptr;

	return Item->Object;
}

const UObject* FWeakObjectPtr::operator->() const
{
	return Get();
}

bool FWeakObjectPtr::operator==(const FWeakObjectPtr& Other) const
{
	return ObjectIndex == Other.ObjectIndex;
}

bool FWeakObjectPtr::operator!=(const FWeakObjectPtr& Other) const
{
	return ObjectIndex != Other.ObjectIndex;
}

bool FWeakObjectPtr::operator==(const class UObject* Other) const
{
	return ObjectIndex == Other->Index;
}

bool FWeakObjectPtr::operator!=(const class UObject* Other) const
{
	return ObjectIndex != Other->Index;
}

template<typename TObjectID>
const UObject* TPersistentObjectPtr<TObjectID>::Get() const
{
	return WeakPtr.Get();
}

template<typename TObjectID>
const UObject* TPersistentObjectPtr<TObjectID>::operator->() const
{
	return WeakPtr.Get();
}


const UObject* SDK::StaticFindObject(const wchar_t* ObjectPath, const UClass* Class)
{
	auto StaticFindObjectInternal = (UObject * (*&)(const UClass*, UObject*, const wchar_t*, bool)) SDK::Offsets::StaticFindObject;
	return StaticFindObjectInternal(Class, nullptr, ObjectPath, false);
}

const UObject* SDK::StaticLoadObject(const wchar_t* ObjectPath, const UClass* InClass, UObject* Outer)
{
	auto StaticLoadObjectInternal = (UObject * (*&)(const UClass*, UObject*, const wchar_t*, const wchar_t*, uint32_t, UObject*, bool)) SDK::Offsets::StaticLoadObject;
	return StaticLoadObjectInternal(InClass, Outer, ObjectPath, nullptr, 0, nullptr, false);
}

const UObject* SDK::FindObject(const wchar_t* ObjectPath, const UClass* Class)
{
	auto Object = StaticFindObject(ObjectPath, Class);
	return Object ? Object : StaticLoadObject(ObjectPath, Class);
}

const UObject* FSoftObjectPtr::InternalGet(const UClass* Class)
{
	if (!this)
		return nullptr;

	auto Object = WeakPtr.Get();

	if (!Object)
	{
		const UObject* Ret = nullptr;

		static auto TopLevelAssetPathStruct = FindStruct("TopLevelAssetPath");
		if (VersionInfo.EngineVersion <= 4.16)
		{
			auto AssetLongPathname = *(FString*)(__int64(this) + offsetof(FSoftObjectPtr, ObjectID));

			if (AssetLongPathname.Num() > 0)
				WeakPtr = Ret = FindObject(AssetLongPathname.CStr(), Class);
		}
		else if (TopLevelAssetPathStruct)
		{
			auto PackageName = *(FName*)(__int64(this) + offsetof(TPersistentObjectPtr, ObjectID));
			auto AssetName = *(FName*)(__int64(this) + offsetof(TPersistentObjectPtr, ObjectID) + 0x4);

			if (PackageName.ComparisonIndex > 0)
			{
				auto FullPath = PackageName.ToWString();
				if (AssetName.ComparisonIndex > 0)
					FullPath += L"." + AssetName.ToWString();
				if (ObjectID.SubPathString.Num() > 0)
					FullPath += L":" + ObjectID.SubPathString.ToWString();

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