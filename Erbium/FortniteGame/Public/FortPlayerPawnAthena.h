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

    DefUHookOg(ServerHandlePickup_);
    DefUHookOg(ServerHandlePickupInfo);
    DefHookOg(bool, FinishedTargetSpline, void*);
    DefUHookOg(ServerSendZiplineState);
    DefUHookOg(OnCapsuleBeginOverlap_);
    static void MovingEmoteStopped(UObject*, FFrame&);
    DefUHookOg(Athena_MedConsumable_Triggered);

    InitPostLoadHooks;
};