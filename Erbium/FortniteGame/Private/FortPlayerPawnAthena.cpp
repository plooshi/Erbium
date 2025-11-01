#include "pch.h"
#include "../Public/FortPlayerPawnAthena.h"
#include "../Public/FortInventory.h"
#include "../Public/FortPlayerControllerAthena.h"
#include "../Public/FortWeapon.h"

struct _Pad_0xC
{
	uint8_t Padding[0xC];
};

struct _Pad_0x18
{
	uint8_t Padding[0x18];
};

struct FFortPickupRequestInfo final
{
public:
	struct FGuid                                  SwapWithItem;                                      // 0x0000(0x0010)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                         FlyTime;                                           // 0x0010(0x0004)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	struct _Pad_0xC                               Direction;                                         // 0x0014(0x000C)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8                                         bPlayPickupSound : 1;                              // 0x0020(0x0001)(BitIndex: 0x00, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bIsAutoPickup : 1;                                 // 0x0020(0x0001)(BitIndex: 0x01, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bUseRequestedSwap : 1;                             // 0x0020(0x0001)(BitIndex: 0x02, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bTrySwapWithWeapon : 1;                            // 0x0020(0x0001)(BitIndex: 0x03, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         Pad_21[0x3];                                       // 0x0021(0x0003)(Fixing Struct Size After Last Property [ Dumper-7 ])
};

struct alignas(0x8) FFortPickupRequestInfoNew final
{
public:
	struct FGuid                                  SwapWithItem;                                      // 0x0000(0x0010)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                         FlyTime;                                           // 0x0010(0x0004)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8 Pad_1[0x4];
	struct _Pad_0x18                              Direction;                                        // 0x0014(0x000C)(ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8                                         bPlayPickupSound : 1;                              // 0x0020(0x0001)(BitIndex: 0x00, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bIsAutoPickup : 1;                                 // 0x0020(0x0001)(BitIndex: 0x01, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bUseRequestedSwap : 1;                             // 0x0020(0x0001)(BitIndex: 0x02, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         bTrySwapWithWeapon : 1;                            // 0x0020(0x0001)(BitIndex: 0x03, PropSize: 0x0001 (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic))
	uint8                                         Pad_2[0x7];                                       // 0x0021(0x0003)(Fixing Struct Size After Last Property [ Dumper-7 ])
};


void AFortPlayerPawnAthena::ServerHandlePickup_(UObject* Context, FFrame& Stack)
{
	AFortPickupAthena* Pickup;
	float InFlyTime;
	FVector InStartDirection;
	bool bPlayPickupSound;
	Stack.StepCompiledIn(&Pickup);
	Stack.StepCompiledIn(&InFlyTime);
	Stack.StepCompiledIn(&InStartDirection);
	Stack.StepCompiledIn(&bPlayPickupSound);
	Stack.IncrementCode();
	auto Pawn = (AFortPlayerPawnAthena*)Context;
	if (!Pawn || !Pickup || Pickup->bPickedUp)
		return;

	Pickup->SetLifeSpan(5.f);
	if (FFortPickupLocationData::HasbPlayPickupSound())
		Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
	Pickup->PickupLocationData.PickupTarget = Pawn;
	Pickup->PickupLocationData.StartDirection = InStartDirection;
	Pickup->PickupLocationData.FlyTime /= Pawn->PickupSpeedMultiplier;
	Pickup->OnRep_PickupLocationData();

	Pickup->bPickedUp = true;
	Pickup->OnRep_bPickedUp();


	Pawn->IncomingPickups.Add(Pickup);
}

void AFortPlayerPawnAthena::ServerHandlePickupInfo(UObject* Context, FFrame& Stack)
{
	bool bTrySwapWithWeapon;
	bool bUseRequestedSwap;
	bool bPlayPickupSound;
	FGuid SwapWithItem;

	AFortPickupAthena* Pickup;
	Stack.StepCompiledIn(&Pickup);
	if (VersionInfo.FortniteVersion >= 20.00)
	{
		FFortPickupRequestInfoNew Params;
		Stack.StepCompiledIn(&Params);
		bTrySwapWithWeapon = Params.bTrySwapWithWeapon;
		bUseRequestedSwap = Params.bUseRequestedSwap;
		bPlayPickupSound = Params.bPlayPickupSound;
		SwapWithItem = Params.SwapWithItem;
	}
	else
	{
		FFortPickupRequestInfo Params;
		Stack.StepCompiledIn(&Params);
		bTrySwapWithWeapon = Params.bTrySwapWithWeapon;
		bUseRequestedSwap = Params.bUseRequestedSwap;
		bPlayPickupSound = Params.bPlayPickupSound;
		SwapWithItem = Params.SwapWithItem;
	}
	Stack.IncrementCode();
	auto Pawn = (AFortPlayerPawnAthena*)Context;

	if (!Pawn || !Pickup || Pickup->bPickedUp)
		return;

	if ((bTrySwapWithWeapon || bUseRequestedSwap) && Pawn->CurrentWeapon && AFortInventory::IsPrimaryQuickbar(((AFortWeapon*)Pawn->CurrentWeapon)->WeaponData) && AFortInventory::IsPrimaryQuickbar(Pickup->PrimaryPickupItemEntry.ItemDefinition))
	{
		auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
		auto SwapEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{ return entry.ItemGuid == SwapWithItem; }, FFortItemEntry::Size());
		PlayerController->SwappingItemDefinition = SwapEntry; // proper af
	}

	Pickup->SetLifeSpan(5.f);
	Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
	Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
	Pickup->PickupLocationData.PickupTarget = Pawn;
	Pickup->PickupLocationData.FlyTime /= Pawn->PickupSpeedMultiplier;
	//Pickup->PickupLocationData.StartDirection = Params.Direction.QuantizeNormal();
	Pickup->OnRep_PickupLocationData();

	Pickup->bPickedUp = true;
	Pickup->OnRep_bPickedUp();


	Pawn->IncomingPickups.Add(Pickup);
}


bool AFortPlayerPawnAthena::FinishedTargetSpline(void* _Pickup)
{
	auto Pickup = (AFortPickupAthena*)_Pickup;

	auto Pawn = (AFortPlayerPawnAthena*)Pickup->PickupLocationData.PickupTarget;
	if (!Pawn)
		return FinishedTargetSplineOG(Pickup);

	auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
	if (!PlayerController)
		return FinishedTargetSplineOG(Pickup);

	if (auto entry = PlayerController->HasSwappingItemDefinition() ? (FFortItemEntry*)PlayerController->SwappingItemDefinition : nullptr)
	{
		AFortInventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), *entry, EFortPickupSourceTypeFlag::GetPlayer(), EFortPickupSpawnSource::GetUnset(), PlayerController->MyFortPawn);
		// SwapEntry(PC, *entry, Pickup->PrimaryPickupItemEntry);
		PlayerController->WorldInventory->Remove(entry->ItemGuid);
		PlayerController->WorldInventory->GiveItem(Pickup->PrimaryPickupItemEntry);
		PlayerController->SwappingItemDefinition = nullptr;
	}
	else
		PlayerController->InternalPickup(&Pickup->PrimaryPickupItemEntry);

	return FinishedTargetSplineOG(Pickup);
}


uint64_t OnRep_ZiplineState = 0;
void AFortPlayerPawnAthena::ServerSendZiplineState(UObject* Context, FFrame& Stack)
{
	FZiplinePawnState State;

	Stack.StepCompiledIn(&State);
	Stack.IncrementCode();

	auto Pawn = (AFortPlayerPawnAthena*)Context;

	if (!Pawn)
		return;

	auto Zipline = Pawn->GetActiveZipline();

	__movsb((PBYTE)&Pawn->ZiplineState, (const PBYTE)&State, FZiplinePawnState::Size());

	((void (*)(AFortPlayerPawnAthena*)) OnRep_ZiplineState)(Pawn);

	/*if (State.bJumped)
	{
		auto Velocity = Pawn->CharacterMovement->Velocity;
		auto VelocityX = Velocity.X * -0.5f;
		auto VelocityY = Velocity.Y * -0.5f;
		Pawn->LaunchCharacterJump(FVector{ VelocityX >= -750 ? min(VelocityX, 750) : -750, VelocityY >= -750 ? min(VelocityY, 750) : -750, 1200 }, false, false, true, true);
	}*/

	static auto ZipLineClass = FindObject<UClass>(L"/Ascender/Gameplay/Ascender/B_Athena_Zipline_Ascender.B_Athena_Zipline_Ascender_C");
	if (auto Ascender = Zipline->Cast<AFortAscenderZipline>(ZipLineClass))
	{
		Ascender->PawnUsingHandle = nullptr;
		Ascender->PreviousPawnUsingHandle = Pawn;
		Ascender->OnRep_PawnUsingHandle();
	}
}


void AFortPlayerPawnAthena::OnCapsuleBeginOverlap_(UObject* Context, FFrame& Stack)
{
	UObject* OverlappedComp;
	AActor* OtherActor;
	UObject* OtherComp;
	int32 OtherBodyIndex;
	bool bFromSweep;
	struct { uint8_t Padding[0x100]; } SweepResult;
	Stack.StepCompiledIn(&OverlappedComp);
	Stack.StepCompiledIn(&OtherActor);
	Stack.StepCompiledIn(&OtherComp);
	Stack.StepCompiledIn(&OtherBodyIndex);
	Stack.StepCompiledIn(&bFromSweep);
	Stack.StepCompiledIn(&SweepResult);
	Stack.IncrementCode();

	auto Pawn = (AFortPlayerPawnAthena*)Context;

	if (!Pawn || !Pawn->Controller)
		return callOG(Pawn, Stack.GetCurrentNativeFunction(), OnCapsuleBeginOverlap, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	auto Pickup = OtherActor->Cast<AFortPickupAthena>();
	if (!Pickup || !Pickup->PrimaryPickupItemEntry.ItemDefinition)
		return callOG(Pawn, Stack.GetCurrentNativeFunction(), OnCapsuleBeginOverlap, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	auto MaxStack = Pickup->PrimaryPickupItemEntry.ItemDefinition->GetMaxStackSize();
	auto itemEntry = ((AFortPlayerControllerAthena*)Pawn->Controller)->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
		{ return entry.ItemDefinition == Pickup->PrimaryPickupItemEntry.ItemDefinition && entry.Count <= MaxStack; }, FFortItemEntry::Size());

	if (Pickup && Pickup->PawnWhoDroppedPickup != Pawn)
	{
		if ((!itemEntry && ((Pickup->PrimaryPickupItemEntry.ItemDefinition->HasbForceAutoPickup() && Pickup->PrimaryPickupItemEntry.ItemDefinition->bForceAutoPickup) || !AFortInventory::IsPrimaryQuickbar(Pickup->PrimaryPickupItemEntry.ItemDefinition))) || (itemEntry && itemEntry->Count < MaxStack))
			Pawn->ServerHandlePickup(Pickup, 0.4f, FVector(), true);
	}

	return callOG(Pawn, Stack.GetCurrentNativeFunction(), OnCapsuleBeginOverlap, OverlappedComp, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}


void AFortPlayerPawnAthena::MovingEmoteStopped(UObject* Context, FFrame& Stack)
{
	Stack.IncrementCode();

	auto Pawn = (AFortPlayerPawnAthena*)Context;
	static auto HasbMovingEmote = Pawn->HasbMovingEmote();
	if (HasbMovingEmote)
		Pawn->bMovingEmote = false;

	static auto HasbMovingEmoteForwardOnly = Pawn->HasbMovingEmoteForwardOnly();
	if (HasbMovingEmoteForwardOnly)
		Pawn->bMovingEmoteForwardOnly = false;

	static auto HasbMovingEmoteFollowingOnly = Pawn->HasbMovingEmoteFollowingOnly();
	if (HasbMovingEmoteFollowingOnly)
		Pawn->bMovingEmoteFollowingOnly = false;
}

class UGA_Athena_MedConsumable_Parent_C : public UObject
{
public:
	UCLASS_COMMON_MEMBERS(UGA_Athena_MedConsumable_Parent_C);

	DEFINE_PROP(PlayerPawn, AFortPlayerPawnAthena*);
	DEFINE_PROP(HealsShields, bool);
	DEFINE_PROP(HealsHealth, bool);
	DEFINE_PROP(HealthHealAmount, float);
};

void AFortPlayerPawnAthena::Athena_MedConsumable_Triggered(UObject* Context, FFrame& Stack)
{
	UGA_Athena_MedConsumable_Parent_C* Consumable = (UGA_Athena_MedConsumable_Parent_C*)Context;

	if (!Consumable || (!Consumable->HealsShields && !Consumable->HealsHealth) || !Consumable->PlayerPawn)
		return Athena_MedConsumable_TriggeredOG(Context, Stack);

	auto PlayerState = (AFortPlayerStateAthena*)Consumable->PlayerPawn->PlayerState;
	static auto ShieldCue = UKismetStringLibrary::Conv_StringToName(FString(L"GameplayCue.Shield.PotionConsumed"));
	static auto HealthCue = UKismetStringLibrary::Conv_StringToName(FString(L"GameplayCue.Athena.Health.HealUsed"));

	auto Handle = PlayerState->AbilitySystemComponent->MakeEffectContext();
	FGameplayTag Tag{};
	FName CueName = Consumable->HealsShields ? ShieldCue : HealthCue;

	if (Consumable->HealsHealth && Consumable->HealsShields)
		if (Consumable->PlayerPawn->GetHealth() + Consumable->HealthHealAmount <= 100)
			CueName = HealthCue;
	Tag.TagName = CueName;

	auto PredictionKey = (FPredictionKey*)malloc(FPredictionKey::Size());
	__stosb((PBYTE)PredictionKey, 0, FPredictionKey::Size());

	PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(Tag, *PredictionKey, Handle);
	PlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(Tag, *PredictionKey, Handle);

	return Athena_MedConsumable_TriggeredOG(Context, Stack);
}

void AFortPlayerPawnAthena::PostLoadHook()
{
	OnRep_ZiplineState = FindOnRep_ZiplineState();

	auto ServerHandlePickupInfoFn = GetDefaultObj()->GetFunction("ServerHandlePickupInfo");

	if (ServerHandlePickupInfoFn)
		Utils::ExecHook(ServerHandlePickupInfoFn, ServerHandlePickupInfo);
	else
	{
		Utils::ExecHook(GetDefaultObj()->GetFunction("ServerHandlePickup"), ServerHandlePickup_);
	}

	Utils::Hook(FindFinishedTargetSpline(), FinishedTargetSpline, FinishedTargetSplineOG);
	Utils::ExecHook(GetDefaultObj()->GetFunction("OnCapsuleBeginOverlap"), OnCapsuleBeginOverlap_, OnCapsuleBeginOverlap_OG);

	Utils::ExecHook(GetDefaultObj()->GetFunction("ServerSendZiplineState"), ServerSendZiplineState);
	Utils::ExecHook(GetDefaultObj()->GetFunction("MovingEmoteStopped"), MovingEmoteStopped);

	if (VersionInfo.EngineVersion >= 4.24 && VersionInfo.EngineVersion < 4.27)
		Utils::ExecHook((UFunction*)FindObject<UFunction>(L"/Game/Athena/Items/Consumables/Parents/GA_Athena_MedConsumable_Parent.GA_Athena_MedConsumable_Parent_C.Triggered_4C02BFB04B18D9E79F84848FFE6D2C32"), Athena_MedConsumable_Triggered, Athena_MedConsumable_TriggeredOG);
}