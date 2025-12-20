#include "pch.h"
#include "../Public/Misc.h"
#include "../Public/Finders.h"
#include <algorithm>
#include "../Public/Configuration.h"
#include "../../FortniteGame/Public/FortPlayerControllerAthena.h"
#include "../../Engine/Public/NetDriver.h"
#include "../../FortniteGame/Public/FortGameMode.h"
#include "../../FortniteGame/Public/FortWeapon.h"
#include "../../FortniteGame/Public/FortKismetLibrary.h"

int Misc::GetNetMode() 
{
	return 1;
}


void* Misc::SendRequestNow(void* Arg1, void* MCPData, int)
{
	if (VersionInfo.EngineVersion < 4.23)
		*(int*)(__int64(MCPData) + (VersionInfo.FortniteVersion >= 4.2 ? 0x28 : 0x60)) = 3; // CXC_Public

	return SendRequestNowOG(Arg1, MCPData, 3); // CXC_Public
}


float Misc::GetMaxTickRate(UEngine* Engine, float DeltaTime, bool bAllowFrameRateSmoothing)
{
	// improper, DS is supposed to do hitching differently
	return (float)FConfiguration::MaxTickRate;
	//return std::clamp(1.f / DeltaTime, 1.f, FConfiguration::MaxTickRate);
}

uint32 Misc::CheckCheckpointHeartBeat()
{
	return -1;
}

void Misc::ApplyHomebaseEffectsOnPlayerSetup(
	__int64* a1,
	__int64 a2,
	__int64 a3,
	__int64 a4,
	UObject* a5,
	char a6,
	unsigned __int8 a7)
{
	auto GameMode = (AFortGameModeAthena*) UWorld::GetWorld()->AuthorityGameMode;
	if (GameMode->HasWarmupRequiredPlayerCount())
	{
		static auto ItemDefOffset = a5->GetOffset("ItemDefinition");
		static auto Commando = FindObject<UObject>(L"/Game/Athena/Heroes/HID_001_Athena_Commando_F.HID_001_Athena_Commando_F");
		static auto Commando2 = FindObject<UObject>(L"/Game/Athena/Heroes/HID_Commando_Athena_01.HID_Commando_Athena_01");
		GetFromOffset<const UObject*>(a5, ItemDefOffset) = Commando ? Commando : Commando2;
	}

	return ApplyHomebaseEffectsOnPlayerSetupOG(a1, a2, a3, a4, a5, a6, a7);
}

bool bEOREnabled = false;
inline void* (*SelectResetOG)(void*) = nullptr;
inline void* (*SelectEditOG)(void*) = nullptr;
inline char (*CompleteBuildingEditInteraction)(void*) = nullptr;

void* SelectEdit(void* a1)
{
	void* result = SelectEditOG(a1);

	if (bEOREnabled)
		CompleteBuildingEditInteraction(a1);

	return result;
}

void* SelectReset(void* a1)
{
	void* result = SelectResetOG(a1);

	if (bEOREnabled)
		CompleteBuildingEditInteraction(a1);

	return result;
}

class UIpNetDriver : public UNetDriver
{
public:
	UCLASS_COMMON_MEMBERS(UIpNetDriver);
};

void PatchAllNetModes(uintptr_t AttemptDeriveFromURL)
{
	Memcury::PE::Address add{ nullptr };

	const auto sizeOfImage = Memcury::PE::GetNTHeaders()->OptionalHeader.SizeOfImage;
	const auto scanBytes = reinterpret_cast<std::uint8_t*>(Memcury::PE::GetModuleBase());

	for (auto i = 0ul; i < sizeOfImage - 5; ++i)
	{
		if (scanBytes[i] == 0xE8 || scanBytes[i] == 0xE9)
		{
			if (Memcury::PE::Address(&scanBytes[i]).RelativeOffset(1).GetAs<void*>() == (void*)AttemptDeriveFromURL)
			{
				add = Memcury::PE::Address(&scanBytes[i]);

				// scan for the read of World->NetDriver

				for (auto j = 0; j > -0x100000; j--) // so we find everything. no func is actually 1mb
				{
					if ((scanBytes[i + j] & 0xF8) == 0x48 && ((scanBytes[i + j + 1] & 0xFC) == 0x80 || (scanBytes[i + j + 1] & 0xF8) == 0x38) && (scanBytes[i + j + 2] & 0xF0) != 0xC0 && (scanBytes[i + j + 2] & 0xF0) != 0xE0 && scanBytes[i + j + 2] != 0x65 && scanBytes[i + j + 2] != 0xBB && scanBytes[i + j + 3] == 0x38 && ((scanBytes[i + j + 1] & 0xFC) != 0x80 || scanBytes[i + j + 4] == 0x0))
					{
						// now, scan for if (NetDriver) return NM_Client;

						bool found = false;
						for (auto k = 4; k < 0x104; k++)
						{
							if (scanBytes[i + j + k] == 0x75)
							{
								auto Scuffness = __int64(&scanBytes[i + j + k + 5]);

								if (*(uint32_t*)Scuffness != 0xF0 && (scanBytes[i + j + k + 4] != 0xC || scanBytes[i + j + k + 5] != 0xB) && scanBytes[i + j + k + 4] != 0x09)
									continue;

								Utils::Patch<uint16_t>(__int64(&scanBytes[i + j + k]), 0x9090);
								if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
									Utils::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
								else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
								{
									DWORD og;
									VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
									*(uint32*)(&scanBytes[i + j]) = 0x90909090;
									*(uint8*)(&scanBytes[i + j + 4]) = 0x90;
									VirtualProtect(&scanBytes[i + j], 5, og, &og);
								}
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 2);
								found = true;
								break;
							}
							else if (scanBytes[i + j + k] == 0x74)	
							{
								auto Scuffness = __int64(&scanBytes[i + j + k]);
								Scuffness = (Scuffness + 2) + *(int8_t*)(Scuffness + 1);
								
								if (*(uint32_t*)(Scuffness + 3) != 0xF0 && (*(uint8_t*)(Scuffness + 2) != 0xC || *(uint8_t*)(Scuffness + 3) != 0xB) && *(uint8_t*)(Scuffness + 2) != 0x09)
									continue;

								Utils::Patch<uint8_t>(__int64(&scanBytes[i + j + k]), 0xeb);
								if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
									Utils::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
								else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
								{
									DWORD og;
									VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
									*(uint32*)(&scanBytes[i + j]) = 0x90909090;
									*(uint8*)(&scanBytes[i + j + 4]) = 0x90;
									VirtualProtect(&scanBytes[i + j], 5, og, &og);
								}
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 1);
								found = true;
								break;
							}
							else if (scanBytes[i + j + k] == 0x0F  && scanBytes[i + j + k + 1] == 0x85)
							{
								auto Scuffness = __int64(&scanBytes[i + j + k + 9]);

								if (*(uint32_t*)Scuffness != 0xF0 && (scanBytes[i + j + k + 8] != 0xC || scanBytes[i + j + k + 9] != 0xB) && scanBytes[i + j + k + 8] != 0x09)
									continue;

								DWORD og;
								VirtualProtect(&scanBytes[i + j + k], 6, PAGE_EXECUTE_READWRITE, &og);
								*(uint32*)(&scanBytes[i + j + k]) = 0x90909090;
								*(uint16*)(&scanBytes[i + j + k + 4]) = 0x9090;
								VirtualProtect(&scanBytes[i + j + k], 6, og, &og);
								if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
									Utils::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
								else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
								{
									DWORD og;
									VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
									*(uint32*)(&scanBytes[i + j]) = 0x90909090;
									*(uint8*)(&scanBytes[i + j + 4]) = 0x90;
									VirtualProtect(&scanBytes[i + j], 5, og, &og);
								}
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 6);
								found = true;
								break;
							}
							else if (scanBytes[i + j + k] == 0x0F && scanBytes[i + j + k + 1] == 0x84)
							{
								auto Scuffness = __int64(&scanBytes[i + j + k]);
								Scuffness = (Scuffness + 6) + *(int32_t*)(Scuffness + 2);

								if (*(uint32_t*)(Scuffness + 3) != 0xF0 && (*(uint8_t*)(Scuffness + 2) != 0xC || *(uint8_t*)(Scuffness + 3) != 0xB) && *(uint8_t*)(Scuffness + 2) != 0x09)
									continue;

								Utils::Patch<uint16_t>(__int64(&scanBytes[i + j + k]), 0xe990);
								if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
									Utils::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
								else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
								{
									DWORD og;
									VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
									*(uint32*)(&scanBytes[i + j]) = 0x90909090;
									*(uint8*)(&scanBytes[i + j + 4]) = 0x90;
									VirtualProtect(&scanBytes[i + j], 5, og, &og);
								}
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
								FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 2);
								found = true;
								break;
							}
						}
						if (found)
							break;
					}
				}
			}
		}
	}
}

bool RetFalse()
{
	return false;
}

class AFortTeamMemberPedestal : public AActor
{
public:
	UCLASS_COMMON_MEMBERS(AFortTeamMemberPedestal);
};

__int64 (*CrashSomethingOG)(__int64 a1, __int64 a2);
__int64 CrashSomething(__int64 a1, __int64 a2)
{
	if (!a1)
		return 0;

	return CrashSomethingOG(a1, a2);
}

class AFortLightweightProjectileManager : public AActor
{
public:
	UCLASS_COMMON_MEMBERS(AFortLightweightProjectileManager);
};

class AFortLightweightProjectileConfig : public AActor
{
public:
	UCLASS_COMMON_MEMBERS(AFortLightweightProjectileConfig);

	DEFINE_PROP(Speed, FScalableFloat);
	DEFINE_PROP(GravityScale, FScalableFloat);
};

struct FSpawnProjectileParams
{
public:
	USCRIPTSTRUCT_COMMON_MEMBERS(FSpawnProjectileParams);

	DEFINE_STRUCT_PROP(SpawnLocation, FVector);
	DEFINE_STRUCT_PROP(SpawnDirection, FRotator);
	DEFINE_STRUCT_PROP(OptionalAssociatedItemDef, UFortItemDefinition*);
	DEFINE_STRUCT_PROP(InitialSpeed, float);
	DEFINE_STRUCT_PROP(MaxSpeed, float);
	DEFINE_STRUCT_PROP(GravityScale, float);
};

class AFortProjectileAthena : public AActor
{
public:
	UCLASS_COMMON_MEMBERS(AFortProjectileAthena);
};

void (*TestOG)(AFortLightweightProjectileManager* ProjectileManager, TWeakObjectPtr<AFortPlayerPawnAthena> WeakPawn, TWeakObjectPtr<AFortWeapon> WeakWeapon, TSubclassOf<AFortLightweightProjectileConfig>& ConfigClass, FVector Location, FVector Direction, uint8_t Type, int a8, int a9);
void Test(AFortLightweightProjectileManager* ProjectileManager, TWeakObjectPtr<AFortPlayerPawnAthena> WeakPawn, TWeakObjectPtr<AFortWeapon> WeakWeapon, TSubclassOf<AFortLightweightProjectileConfig>& ConfigClass, FVector Location, FVector Direction, uint8_t Type, int a8, int a9)
{
	TestOG(ProjectileManager, WeakPawn, WeakWeapon, ConfigClass, Location, Direction, Type, a8, a9);

	auto Config = (AFortLightweightProjectileConfig*)ConfigClass->GetDefaultObj();

	auto Params = (FSpawnProjectileParams*)malloc(FSpawnProjectileParams::Size());
	memset(Params, 0, FSpawnProjectileParams::Size());

	Params->SpawnLocation = Location;

	const double RAD_TO_DEG = 57.29577951308232;

	auto Weapon = WeakWeapon.Get();

	auto Pitch = asin(Direction.Z) * RAD_TO_DEG;
	auto Yaw = atan2(Direction.Y, Direction.X) * RAD_TO_DEG;
	Params->SpawnDirection.Pitch = Pitch;
	Params->SpawnDirection.Yaw = Yaw;
	Params->SpawnDirection.Roll = 0;
	Params->OptionalAssociatedItemDef = Weapon->WeaponData;
	Params->InitialSpeed = Config->Speed.Evaluate();
	Params->MaxSpeed = Params->InitialSpeed;
	Params->GravityScale = Config->GravityScale.Evaluate();

	UFortKismetLibrary::SpawnProjectileWithParams(AFortProjectileAthena::StaticClass(), Weapon, *Params);
	free(Params);
}

struct FTickFunction
{
public:
	USCRIPTSTRUCT_COMMON_MEMBERS(FTickFunction);

	DEFINE_STRUCT_BITFIELD_PROP(bAllowTickOnDedicatedServer);
};

void (*Test2OG)(FTickFunction* _this, ULevel* Level);
void Test2(FTickFunction* _this, ULevel* Level)
{
	if (!_this->bAllowTickOnDedicatedServer)
	{
		return;
	}
	return Test2OG(_this, Level);
}

void Ohio(ABuildingProp_LockDevice* _this, AFortPlayerControllerAthena* ControllerInstigator)
{
	printf("Called[LockProp: %s]\n", _this->Name.ToString().c_str());
}

bool Listen()
{
	printf("UWorld::Listen\n");
	auto World = UWorld::GetWorld();
	auto Engine = UEngine::GetEngine();
	auto NetDriverName = FName(L"GameNetDriver");
	auto GameMode = (AFortGameModeAthena*)World->AuthorityGameMode;

	if (GameMode->HasbEnableReplicationGraph())
		GameMode->bEnableReplicationGraph = true;

	UNetDriver* NetDriver = nullptr;
	if (VersionInfo.FortniteVersion >= 16.00)
	{
		void* WorldCtx = ((void* (*)(UEngine*, UWorld*)) FindGetWorldContext())(Engine, World);
		World->NetDriver = NetDriver = ((UNetDriver * (*)(UEngine*, void*, FName, int)) FindCreateNetDriverWorldContext())(Engine, WorldCtx, NetDriverName, 0);
	}
	else
		World->NetDriver = NetDriver = ((UNetDriver * (*)(UEngine*, UWorld*, FName)) FindCreateNetDriver())(Engine, World, NetDriverName);

	if (!NetDriver)
		return false;

	if (VersionInfo.FortniteVersion >= 20)
		NetDriver->NetServerMaxTickRate = 30;

	NetDriver->NetDriverName = NetDriverName;
	NetDriver->World = World;

	if (VersionInfo.EngineVersion >= 5.3 && FConfiguration::bEnableIris)
	{
		*(bool*)(__int64(&NetDriver->ReplicationDriver) + 0x11) = true;
	}

	NetDriver->NetDriverName = NetDriverName;
	NetDriver->World = World;

	auto InitListen = (bool (*)(UNetDriver*, UWorld*, FURL*, bool, FString&)) FindInitListen();
	auto SetWorld = (void (*)(UNetDriver*, UWorld*)) FindSetWorld();

	SetWorld(NetDriver, World);
	for (int i = 0; i < World->LevelCollections.Num(); i++)
	{
		auto& LevelCollection = World->LevelCollections.Get(i, FLevelCollection::Size());

		LevelCollection.NetDriver = NetDriver;
	}

	auto URL = (FURL*)malloc(FURL::Size());
	memset((PBYTE)URL, 0, FURL::Size());
	URL->Port = FConfiguration::Port;


	FString Err;
	if (!InitListen(NetDriver, World, URL, false, Err))
	{
		printf("Failed to listen!");

		free(URL);

		return false;
	}
	SetWorld(NetDriver, World);

	free(URL);

	return true;
}

struct FFortBotCosmeticItemSetDataTableRow
{
public:
	USCRIPTSTRUCT_COMMON_MEMBERS(FFortBotCosmeticItemSetDataTableRow);

	DEFINE_STRUCT_PROP(SetTag, FGameplayTag);
	DEFINE_STRUCT_PROP(CharacterAssetId, FPrimaryAssetId);
	DEFINE_STRUCT_PROP(BackpackAssetId, FPrimaryAssetId);
	DEFINE_STRUCT_PROP(Weight, float);
};

class UFortAthenaAIBotCosmeticLibraryData : public UObject
{
public:
	UCLASS_COMMON_MEMBERS(UFortAthenaAIBotCosmeticLibraryData);

	DEFINE_PROP(PredefineSetsDataTable, TSoftObjectPtr<UDataTable>);
};

class UFortAthenaAIBotCharacterCustomization : public UObject
{
public:
	UCLASS_COMMON_MEMBERS(UFortAthenaAIBotCharacterCustomization);

	DEFINE_PROP(CustomizationLoadout, FFortAthenaLoadout);
};

class UFortAthenaAIBotCustomizationData : public UObject
{
public:
	UCLASS_COMMON_MEMBERS(UFortAthenaAIBotCustomizationData);

	DEFINE_PROP(CosmeticCustomizationLibrary, TSoftObjectPtr<UFortAthenaAIBotCosmeticLibraryData>);
	DEFINE_PROP(OverrideCosmeticMode, uint8_t);
	DEFINE_PROP(CharacterCustomization, UFortAthenaAIBotCharacterCustomization*);
};

template <typename T>
std::pair<FName, T*> PickWeighted(UEAllocatedMap<FName, T*>& Map, float (*RandFunc)(float), bool bCheckZero = true)
{
	float TotalWeight = std::accumulate(Map.begin(), Map.end(), 0.0f, [&](float acc, std::pair<FName, T*> p)
		{ return acc + p.second->Weight; });
	float RandomNumber = RandFunc(TotalWeight);

	for (auto& Element : Map)
	{
		float Weight = Element.second->Weight;
		if (bCheckZero && Weight == 0)
			continue;

		if (RandomNumber <= Weight) return Element;

		RandomNumber -= Weight;
	}

	std::pair<FName, T*> None;
	return None;
}

void InitializeCosmeticLoadout(UFortAthenaAIBotCustomizationData* BotData, AFortPlayerPawnAthena* Pawn, FFortAthenaLoadout& OutLoadout, FGameplayTag* PredefinedCosmeticSetTag)
{
	OutLoadout = BotData->CharacterCustomization->CustomizationLoadout;

	if (BotData->OverrideCosmeticMode == 1)
	{
		UEAllocatedMap<FName, FFortBotCosmeticItemSetDataTableRow*> LibraryRowMap;
		FName& CosmeticTag = PredefinedCosmeticSetTag->TagName;

		for (auto& [Key, Val] : (TMap<FName, FFortBotCosmeticItemSetDataTableRow*>&)BotData->CosmeticCustomizationLibrary->PredefineSetsDataTable->RowMap)
			if (Val->SetTag.TagName == CosmeticTag)
				LibraryRowMap[Key] = Val;

		auto LibraryRowPair = PickWeighted(LibraryRowMap, [](float Total)
			{ return ((float)rand() / 32767) * Total; });
		auto& LibraryRow = LibraryRowPair.second;

		OutLoadout.Character = (UAthenaCharacterItemDefinition*)UKismetSystemLibrary::GetObjectFromPrimaryAssetId(LibraryRow->CharacterAssetId);
		if (LibraryRow->BackpackAssetId.PrimaryAssetType.IsValid())
			OutLoadout.Backpack = (UAthenaCharacterPartItemDefinition*) UKismetSystemLibrary::GetObjectFromPrimaryAssetId(LibraryRow->BackpackAssetId);
	}

	UEAllocatedMap<uint8_t, const UCustomCharacterPart*> PartMap;

	if (auto HeroDefinition = OutLoadout.Character->HeroDefinition)
		for (auto& SoftSpec : HeroDefinition->Specializations)
		{
			auto Specialization = SoftSpec.Get();

			if (Specialization)
				for (auto& PartSoft : Specialization->CharacterParts)
				{
					auto Part = PartSoft.Get();

					PartMap[Part->CharacterPartType] = Part;
				}
		}

	if (OutLoadout.Backpack)
		for (auto& Part : OutLoadout.Backpack->CharacterParts)
			PartMap[Part->CharacterPartType] = Part;

	for (int i = 0; i < OutLoadout.CharacterVariantChannels.Num(); i++)
	{
		auto& VariantChannel = OutLoadout.CharacterVariantChannels.Get(i, FMcpVariantChannelInfo::Size());
		auto CosmeticForVariant = (UAthenaCosmeticItemDefinition*)VariantChannel.ItemVariantIsUsedFor;

		for (auto& ItemVariant : CosmeticForVariant->ItemVariants)
			if (auto PartVariant = ItemVariant->Cast<UFortCosmeticCharacterPartVariant>())
				for (int i = 0; i < PartVariant->PartOptions.Num(); i++)
				{
					auto& PartOption = PartVariant->PartOptions.Get(i, FPartVariantDef::Size());

					if (VariantChannel.ActiveVariantTag.TagName == PartOption.CustomizationVariantTag.TagName)
						for (auto& PartSoft : PartOption.VariantParts)
						{
							auto Part = PartSoft.Get();

							PartMap[Part->CharacterPartType] = Part;
						}
				}
	}

	for (auto& [PartType, Part] : PartMap)
		Pawn->ServerChoosePart(PartType, Part);
}

void Misc::Hook()
{
	if (VersionInfo.FortniteVersion == 23.00 || (VersionInfo.FortniteVersion >= 24.30 && VersionInfo.FortniteVersion != 28.30 && VersionInfo.FortniteVersion != 29.40) || VersionInfo.FortniteVersion >= 30)
	{
		auto AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B C1").Get();
		if (!AttemptDeriveFromURL)
			AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8B D1").Get();
		if (!AttemptDeriveFromURL)
			AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B D1").Get();
		if (!AttemptDeriveFromURL)
			AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B D1").Get();

		Utils::Hook(AttemptDeriveFromURL, GetNetMode);
		PatchAllNetModes(AttemptDeriveFromURL);
	}
	else if (VersionInfo.FortniteVersion >= 28)
	{
		auto AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B C1").Get();
		if (!AttemptDeriveFromURL)
			AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8B D1").Get();
		if (!AttemptDeriveFromURL)
			AttemptDeriveFromURL = Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 4C 8B D1").Get();

		Utils::Hook(AttemptDeriveFromURL, GetNetMode);
		PatchAllNetModes(AttemptDeriveFromURL);

		Utils::Hook(FindGetNetMode(), GetNetMode);
	}
	else
		Utils::Hook(FindGetNetMode(), GetNetMode);

	Utils::Hook(FindSendRequestNow(), SendRequestNow, SendRequestNowOG);
	Utils::Hook(FindGetMaxTickRate(), GetMaxTickRate);
	if (VersionInfo.FortniteVersion >= 17)
	{
		auto pattern = Memcury::Scanner::FindPattern("48 89 5C 24 10 48 89 6C 24 20 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B F9").Get();

		if (!pattern)
			pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B E9").Get();

		if (!pattern)
			pattern = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 55 41 56 48 81 EC ? ? ? ? 65 48 8B 04 25").Get();

		Utils::Hook(pattern, CheckCheckpointHeartBeat);
	}
	if (VersionInfo.EngineVersion < 4.20)
	{
		auto ApplyHomebaseEffectsOnPlayerSetupAddr = Memcury::Scanner::FindPattern("40 55 53 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 00 4C 8B").Get();

		Utils::Hook(ApplyHomebaseEffectsOnPlayerSetupAddr, ApplyHomebaseEffectsOnPlayerSetup, ApplyHomebaseEffectsOnPlayerSetupOG);
	}
	if (VersionInfo.FortniteVersion >= 25 && VersionInfo.FortniteVersion < 28)
	{
		Utils::Hook(Memcury::Scanner::FindPattern("48 89 5C ? ? 57 48 83 EC ? 48 8B D1 48 85 C9 74 ?").Get(), RetFalse);
	}


	auto PedestalBeginPlay = Memcury::Scanner::FindStringRef(L"AFortTeamMemberPedestal::BeginPlay - Begun play on pedestal %s", true, 0, VersionInfo.EngineVersion >= 5.0).Get();

	if (PedestalBeginPlay)
	{
		uint64_t RealBeginPlay = 0;
		for (int i = 0; i < 1000; i++)
		{
			auto Ptr = (uint8_t*)(PedestalBeginPlay - i);

			if (*Ptr == 0x48 && *(Ptr + 1) == 0x89 && *(Ptr + 2) == 0x5c)
			{
				RealBeginPlay = (uint64_t)Ptr;
				break;
			}
			else if (*Ptr == 0x40 && *(Ptr + 1) == 0x53 && *(Ptr + 2) == 0x41 && *(Ptr + 3) == 0x56)
			{
				RealBeginPlay = (uint64_t)Ptr;
				break;
			}
		}

		auto ActorVft = AFortTeamMemberPedestal::GetDefaultObj()->Vft;

		for (int i = 0; i < 0x500; i++)
		{
			if (ActorVft[i] == (void*)RealBeginPlay)
			{
				Utils::Hook<AFortTeamMemberPedestal>(uint32_t(i), AActor::GetDefaultObj()->Vft[i]);
				break;
			}
		}
	}
	
	if (VersionInfo.FortniteVersion >= 23)
	{
		auto pattern = Memcury::Scanner::FindPattern("48 8B 01 FF 90 ? ? ? ? 48 8B 8B ? ? ? ? 48 85 C9 74 ? 48 8B 01 FF 90 ? ? ? ? 48 8D 8B");

		auto patchPoint = pattern.ScanFor(VersionInfo.EngineVersion < 5.5 ? std::vector<uint8_t>{ 0x48, 0x89, 0x5C } : std::vector<uint8_t>{ 0x40, 0x53 }, false).ScanFor({0x83, 0xF8, 0x02}).Get();

		Utils::Patch<uint8_t>(patchPoint + 2, 0x1);
	}

	if (VersionInfo.FortniteVersion >= 24.30)
	{
		auto sig = VersionInfo.EngineVersion == 5.3 ? Memcury::Scanner::FindPattern("40 53 48 83 EC ? 48 8B DA 49 8B D0 E8 ? ? ? ? 48 85 C0 0F 85 ? ? ? ? 48 39 83").Get() : Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 4C 8D B1 ? ? ? ? 33 DB 49 8D 7E").Get();

		if (!sig)
			sig = Memcury::Scanner::FindPattern("40 53 48 83 EC ? 48 8B DA 48 8B D1 48 81 C1 ? ? ? ? E8 ? ? ? ? 48 85 C0 74 ? 4C 8B 0B 45 33 C0").Get();
		
		Utils::Hook(sig, CrashSomething, CrashSomethingOG);
	}

	//Utils::Hook(ImageBase + 0x1CE85F4, Test);
	//Utils::Hook(ImageBase + 0x2788BEC, Test, TestOG);
	//Utils::Hook(Memcury::Scanner::FindPattern("48 89 5C 24 ?? 57 48 83 EC ?? 48 8B DA 48 8B F9 E8 ?? ?? ?? ?? 84 C0 75 ?? 48 83 79").Get(), Test2, Test2OG);
	/*if (ABuildingProp_LockDevice::StaticClass())
	{
		auto Fn = ABuildingProp_LockDevice::GetDefaultObj()->GetFunction("UnlockObject");

		Utils::Hook<ABuildingProp_LockDevice>(Fn->GetVTableIndex(), Ohio);
	}

	auto ListenCall = FindListenCall();

	if (ListenCall)
	{
		auto OverrideFunc = __int64(DefaultObjImpl("FortHUDContext")->GetFunction("EnterCameraMode")->ExecFunction);

		Utils::Hook(OverrideFunc, Listen);

		auto NewRel = uint32(OverrideFunc - (ListenCall + 5));

		Utils::Patch<uint32>(ListenCall + 1, NewRel);
	}*/

	if (VersionInfo.FortniteVersion >= 11 && VersionInfo.FortniteVersion < 16)
	{
		auto NearGetOverrideCosmeticLoadout = Memcury::Scanner::FindPattern("4D 8B CD 4C 8D 45 ? 48 8B D6");

		if (!NearGetOverrideCosmeticLoadout.IsValid())
			NearGetOverrideCosmeticLoadout = Memcury::Scanner::FindPattern("4C 8D 45 ? 48 8B D3 48 8B CF E8 ? ? ? ? 0F B6 57");

		if (!NearGetOverrideCosmeticLoadout.IsValid())
			NearGetOverrideCosmeticLoadout = Memcury::Scanner::FindPattern("4C 8D 45 ? 48 8B D3 48 8B CF E8 ? ? ? ? 0F B6 4F");

		if (NearGetOverrideCosmeticLoadout.IsValid())
		{
			auto Rel32 = NearGetOverrideCosmeticLoadout.ScanFor({ 0xE8 }).Get();

			auto OverrideFunc = __int64(DefaultObjImpl("FortHUDContext")->GetFunction("EnterCursorMode")->ExecFunction);

			Utils::Hook(OverrideFunc, InitializeCosmeticLoadout);

			auto NewRel = uint32(OverrideFunc - (Rel32 + 5));

			Utils::Patch<uint32>(Rel32 + 1, NewRel);
		}
	}
}
