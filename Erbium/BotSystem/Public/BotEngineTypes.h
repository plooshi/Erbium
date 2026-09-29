#pragma once
#include "../../pch.h"
#include "../../FortniteGame/Public/FortPlayerPawnAthena.h"
#include "../../FortniteGame/Public/FortPlayerStateAthena.h"

// Engine classes the bot system needs that Erbium's SDK does not declare yet.
//
// Like everything in the SDK these are thin views over the real engine objects: properties and
// UFunctions are looked up by name at runtime (see SDK/Defs.h), so nothing here depends on a
// particular engine version's memory layout. Every function used here is a standard reflected
// (BlueprintCallable) engine function that exists across UE 4.24 - 5.x. Use Has*() / the
// capability checks in BotNavigation before relying on one.

// AController: common base of APlayerController and AAIController.
class AController : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(AController);

    DEFINE_PROP(PlayerState, AFortPlayerStateAthena*);
    DEFINE_PROP(Pawn, AFortPlayerPawnAthena*);

    DEFINE_FUNC(Possess, void);
    DEFINE_FUNC(UnPossess, void);
    DEFINE_FUNC(OnRep_PlayerState, void);
    DEFINE_FUNC(StopMovement, void);
    DEFINE_FUNC(LineOfSightTo, bool);
    DEFINE_FUNC(GetControlRotation, FRotator);
    DEFINE_FUNC(SetControlRotation, void);
};

// UBrainComponent: runs behavior trees / state machines on an AI controller.
class UBrainComponent : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UBrainComponent);

    DEFINE_FUNC(StopLogic, void);
};

// AAIController: server-side controller. Unlike a client-less APlayerController on a dedicated server,
// its pawn counts as locally controlled, so the movement component simulates it on the server.
class AAIController : public AController
{
public:
    UCLASS_COMMON_MEMBERS(AAIController);

    DEFINE_PROP(BrainComponent, UBrainComponent*);

    DEFINE_FUNC(MoveToLocation, uint8);
    DEFINE_FUNC(MoveToActor, uint8);
    DEFINE_FUNC(GetMoveStatus, uint8);
    DEFINE_FUNC(K2_SetFocus, void);
    DEFINE_FUNC(K2_SetFocalPoint, void);
    DEFINE_FUNC(K2_ClearFocus, void);
};

// APawn / ACharacter functions, called on the Fortnite player pawn.
class ACharacter : public AActor
{
public:
    UCLASS_COMMON_MEMBERS(ACharacter);

    DEFINE_BITFIELD_PROP(bHidden); // AActor::bHidden, looked up on the pawn

    DEFINE_FUNC(AddMovementInput, void);
    DEFINE_FUNC(SetActorEnableCollision, void);
    DEFINE_FUNC(Jump, void);
    DEFINE_FUNC(StopJumping, void);
    DEFINE_FUNC(LaunchCharacter, void); // (FVector LaunchVelocity, bool bXYOverride, bool bZOverride)
};

// NavigationSystemV1 (UE 4.20+). Static Kismet-style helpers.
class UNavigationSystemV1 : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UNavigationSystemV1);

    DEFINE_STATIC_FUNC(K2_GetRandomReachablePointInRadius, bool);
    DEFINE_STATIC_FUNC(K2_ProjectPointToNavigation, bool);
};
