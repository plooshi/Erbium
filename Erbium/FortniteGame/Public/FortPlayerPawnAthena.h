#pragma once
#include "../../pch.h"
#include "GameplayTagContainer.h"

class UCharacterMovementComponent : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UCharacterMovementComponent);

    DEFINE_PROP(Velocity, FVector);
};

struct FZiplinePawnState
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FZiplinePawnState);

    DEFINE_STRUCT_PROP(bJumped, bool);

    uint8_t Padding[0x100];
};

class AFortAscenderZipline : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortAscenderZipline);

    DEFINE_NEWOBJ_PROP(PawnUsingHandle, AActor);
    DEFINE_PROP(PreviousPawnUsingHandle, TWeakObjectPtr<AActor>);

    DEFINE_FUNC(OnRep_PawnUsingHandle, void);
    DEFINE_FUNC(OnZipliningStarted, void);
    DEFINE_FUNC(OnZipliningStopped, void);
};

struct FFortGameplayAttributeData
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortGameplayAttributeData);

    DEFINE_STRUCT_PROP(CurrentValue, float);
    DEFINE_STRUCT_PROP(BaseValue, float);
    DEFINE_STRUCT_PROP(Minimum, float);
    DEFINE_STRUCT_PROP(Maximum, float);
    DEFINE_STRUCT_PROP(UnclampedBaseValue, float);
    DEFINE_STRUCT_PROP(UnclampedCurrentValue, float);
};

class UFortHealthSet : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortHealthSet);

    DEFINE_PROP(Health, FFortGameplayAttributeData);

    DEFINE_FUNC(OnRep_Health, void);
};

struct FDamagerInfo
{
public:
    AActor* DamageCauser;
    int32 DamageAmount;
    FGameplayTagContainer SourceTags;
};

class AFortPlayerPawnAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPlayerPawnAthena);

    DEFINE_PROP(CurrentWeapon, AActor*); // everything breaks if we include FortWeapon.h so
    DEFINE_PROP(PreviousWeapon, AActor*);
    DEFINE_PROP(Controller, AActor*);
    DEFINE_PROP(IncomingPickups, TArray<AActor*>);
    DEFINE_PROP(CharacterMovement, UCharacterMovementComponent*);
    DEFINE_PROP(ZiplineState, FZiplinePawnState);
    DEFINE_BITFIELD_PROP(bMovingEmote);
    DEFINE_PROP(EmoteWalkSpeed, float);
    DEFINE_BITFIELD_PROP(bMovingEmoteForwardOnly);
    DEFINE_BITFIELD_PROP(bMovingEmoteFollowingOnly);
    DEFINE_PROP(LastFallDistance, float);
    DEFINE_PROP(GameplayTags, FGameplayTagContainer);
    DEFINE_BITFIELD_PROP(bIsInAnyStorm);
    DEFINE_BITFIELD_PROP(bIsInsideSafeZone);
    DEFINE_PROP(AIControllerClass, TSubclassOf<AActor>);
    DEFINE_PROP(PlayerState, AActor*);
    DEFINE_PROP(BaseEyeHeight, float);
    DEFINE_PROP(OnHeldObjectPickedUp, TMulticastInlineDelegate<void(AActor*)>);
    DEFINE_PROP(OnHeldObjectDropped, TMulticastInlineDelegate<void(AActor*)>);
    DEFINE_PROP(OnEnteredAircraft, TMulticastInlineDelegate<void()>);
    DEFINE_PROP(PickupSpeedMultiplier, float);
    DEFINE_PROP(HeldObject, TWeakObjectPtr<AActor>);
    DEFINE_PROP(RepActiveMovementModeExtension, void*);
    DEFINE_BITFIELD_PROP(bIsPlayingEmote);
    DEFINE_PROP(HealthSet, UFortHealthSet*);
    DEFINE_PROP(CurrentWeaponList, TArray<AActor*>);
    DEFINE_PROP(bShouldDropItemsOnDeath, bool);
    DEFINE_PROP(MoveSoundStimulusBroadcastInterval, uint16_t);
    DEFINE_PROP(Damagers, TArray<FDamagerInfo>);
    DEFINE_PROP(LastReplicatedEmoteExecuted, UObject*);
    DEFINE_PROP(Mesh, UActorComponent*);
    DEFINE_BITFIELD_PROP(bIsSkydiving);

    DEFINE_FUNC(BeginSkydiving, void);
    DEFINE_FUNC(GetHealth, float);
    DEFINE_FUNC(GetShield, float);
    DEFINE_FUNC(SetHealth, void);
    DEFINE_FUNC(SetShield, void);
    DEFINE_FUNC(SetMaxHealth, void);
    DEFINE_FUNC(EquipWeaponDefinition, AActor*);
    DEFINE_FUNC(LaunchCharacterJump, void);
    DEFINE_FUNC(OnCapsuleBeginOverlap, void);
    DEFINE_FUNC(ServerHandlePickup, void);
    DEFINE_FUNC(IsDBNO, bool);
    DEFINE_FUNC(PickUpActor, void);
    DEFINE_FUNC(OnRep_IsInAnyStorm, void);
    DEFINE_FUNC(OnRep_IsInsideSafeZone, void);
    DEFINE_FUNC(OnRep_PlayerState, void);
    DEFINE_FUNC(ServerSetAttachment, void);
    DEFINE_FUNC(GetActiveZipline, AFortAscenderZipline*);
    DEFINE_FUNC(ServerOnExitVehicle, void);
    DEFINE_FUNC(SetInVortex, void);
    DEFINE_FUNC(ClientInternalEquipWeapon, void);
    DEFINE_FUNC(ServerInternalEquipWeapon, void);
    DEFINE_FUNC(SetGravityMultiplier, void);
    DEFINE_FUNC(OnRep_LastReplicatedEmoteExecuted, void);
    DEFINE_FUNC(EmoteStopped, void);
    DEFINE_FUNC(ServerChoosePart, void);

    DefUHookOg(ServerHandlePickup_);
    DefUHookOg(ServerHandlePickupInfo);
    DefHookOg(bool, FinishedTargetSpline, void*);
    DefUHookOg(ServerSendZiplineState);
    DefUHookOg(OnCapsuleBeginOverlap_);
    static void MovingEmoteStopped(UObject*, FFrame&);
    DefUHookOg(Athena_MedConsumable_Triggered);
    DefUHookOg(ServerOnExitVehicle_);
    DefUHookOg(EmoteStopped_);
    static void ServerHandlePickupWithRequestedSwap(UObject*, FFrame&);
    DefHookOg(void, EndSkydiving, AFortPlayerPawnAthena*);

    InitPostLoadHooks;
};