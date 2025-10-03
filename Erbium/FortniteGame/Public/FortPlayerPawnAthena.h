#pragma once
#include "../../pch.h"
#include "../../Erbium/Public/Utils.h"
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

class AFortPlayerPawnAthena : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AFortPlayerPawnAthena);

    DEFINE_PROP(CurrentWeapon, AActor*); // everything breaks if we include FortWeapon.h so
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

    DEFINE_FUNC(BeginSkydiving, void);
    DEFINE_FUNC(SetHealth, void);
    DEFINE_FUNC(SetShield, void);
    DEFINE_FUNC(EquipWeaponDefinition, AActor*);
    DEFINE_FUNC(LaunchCharacterJump, void);
    DEFINE_FUNC(OnCapsuleBeginOverlap, void);
    DEFINE_FUNC(ServerHandlePickup, void);
    DEFINE_FUNC(IsDBNO, bool);
    DEFINE_FUNC(PickUpActor, void);

    DefUHookOg(ServerHandlePickup_);
    DefUHookOg(ServerHandlePickupInfo);
    DefHookOg(bool, FinishedTargetSpline, void*);
    DefUHookOg(ServerSendZiplineState);
    DefUHookOg(OnCapsuleBeginOverlap_);
    static void MovingEmoteStopped(UObject*, FFrame&);

    InitHooks;
};