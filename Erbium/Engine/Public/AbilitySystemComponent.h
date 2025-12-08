#pragma once
#include "../../pch.h"

struct FGameplayAbilitySpecHandle
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameplayAbilitySpecHandle);

    int Handle;
};

class UFortGameplayAbility : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortGameplayAbility);

    DEFINE_FUNC(K2_ExecuteGameplayCue, void);
    DEFINE_FUNC(K2_ExecuteGameplayCueWithParams, void);
    DEFINE_FUNC(GetAbilitySystemComponentFromActorInfo, UObject*);
    DEFINE_FUNC(K2_AddGameplayCueWithParams, void);
    DEFINE_FUNC(K2_AddGameplayCue, void);

    DefUHookOg(K2_ExecuteGameplayCue_);
    DefUHookOg(K2_ExecuteGameplayCueWithParams_);
    DefUHookOg(K2_AddGameplayCueWithParams_);
    DefUHookOg(K2_AddGameplayCue_);
};

struct FPredictionKey
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FPredictionKey);
    uint8_t Padding[0x18];

    DEFINE_STRUCT_PROP(Current, int16);
};

struct FGameplayAbilitySpec : public FFastArraySerializerItem
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameplayAbilitySpec);

    DEFINE_STRUCT_PROP(Handle, FGameplayAbilitySpecHandle);
    DEFINE_STRUCT_PROP(Ability, UFortGameplayAbility*);
    DEFINE_STRUCT_PROP(Level, int32);
    DEFINE_STRUCT_PROP(InputID, int32);
    DEFINE_STRUCT_NEWOBJ_PROP(SourceObject, UObject);
    DEFINE_STRUCT_BITFIELD_PROP(InputPressed);
};

struct FGameplayAbilitySpecContainer : public FFastArraySerializer
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameplayAbilitySpecContainer);

    DEFINE_STRUCT_PROP(Items, TArray<FGameplayAbilitySpec>);
};

class UGameplayEffect : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UGameplayEffect);
};


struct FGameplayEffectApplicationInfoHard
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameplayEffectApplicationInfoHard);

    DEFINE_STRUCT_PROP(GameplayEffect, TSubclassOf<UGameplayEffect>);
    DEFINE_STRUCT_PROP(Level, float);
};

class UFortAbilitySet : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortAbilitySet);
    
    DEFINE_PROP(GameplayAbilities, TArray<TSubclassOf<UFortGameplayAbility>>);
    DEFINE_PROP(GrantedGameplayEffects, TArray<FGameplayEffectApplicationInfoHard>);
};

struct FGameplayEffectContextHandle
{
    uint8_t Padding[0x18];
};

struct FGameplayCueParameters
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameplayCueParameters);
    uint8_t Padding[0xD0];

    DEFINE_STRUCT_PROP(EffectContext, FGameplayEffectContextHandle);
};


struct FActiveGameplayEffectHandle
{
public:
    int32 Handle;
    bool bPassedFiltersAndWasExecuted;
    uint8 Pad_5[0x3];
};

struct FGameplayEffectSpec
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameplayEffectSpec);

    DEFINE_STRUCT_PROP(Def, UGameplayEffect*);
};


struct FActiveGameplayEffect
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FActiveGameplayEffect);

    DEFINE_STRUCT_PROP(Spec, FGameplayEffectSpec);
};


struct FActiveGameplayEffectsContainer
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FActiveGameplayEffectsContainer);

    DEFINE_STRUCT_PROP(GameplayEffects_Internal, TArray<FActiveGameplayEffect>);
};

class IFortAbilitySystemInterface : public IInterface
{
public:
    UCLASS_COMMON_MEMBERS(IFortAbilitySystemInterface);
};

class UAbilitySystemComponent : public UActorComponent
{
public:
    UCLASS_COMMON_MEMBERS(UAbilitySystemComponent);

    DEFINE_PROP(ActivatableAbilities, FGameplayAbilitySpecContainer);
    DEFINE_PROP(ActiveGameplayEffects, FActiveGameplayEffectsContainer);

    DEFINE_FUNC(ClientActivateAbilityFailed, void);
    DEFINE_FUNC(NetMulticast_InvokeGameplayCueAdded, void);
    DEFINE_FUNC(NetMulticast_InvokeGameplayCueAdded_WithParams, void);
    DEFINE_FUNC(NetMulticast_InvokeGameplayCueExecuted, void);
    DEFINE_FUNC(NetMulticast_InvokeGameplayCueExecuted_WithParams, void);
    DEFINE_FUNC(MakeEffectContext, FGameplayEffectContextHandle);
    DEFINE_FUNC(BP_ApplyGameplayEffectToSelf, FActiveGameplayEffectHandle);
    DEFINE_FUNC(UpdateActiveGameplayEffectSetByCallerMagnitude, void);
    DEFINE_FUNC(SetActiveGameplayEffectLevel, void);

    FGameplayAbilitySpecHandle GiveAbility(const UObject* Ability, UObject* SourceObject = nullptr);
    void GiveAbilitySet(const UFortAbilitySet* Set);
    static void InternalServerTryActivateAbility(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, bool, FPredictionKey*, void*);

    InitHooks;
};