#pragma once
#include "../../pch.h"

struct FGameplayAbilitySpecHandle
{
public:
    int Handle;
};

class UFortGameplayAbility : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortGameplayAbility);
};

struct FPredictionKey
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FPredictionKey);

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
    DEFINE_STRUCT_PROP(InputPressed, uint8);
};

struct FGameplayAbilitySpecContainer : public FFastArraySerializer
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FGameplayAbilitySpecContainer);

    DEFINE_STRUCT_PROP(Items, TArray<FGameplayAbilitySpec>);
};

class UFortAbilitySet : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortAbilitySet);
    
    DEFINE_PROP(GameplayAbilities, TArray<TSubclassOf<UFortGameplayAbility>>);
};

struct FGameplayEffectContextHandle
{
    uint8_t Padding[0x18];
};

class UAbilitySystemComponent : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UAbilitySystemComponent);

    DEFINE_PROP(ActivatableAbilities, FGameplayAbilitySpecContainer);

    DEFINE_FUNC(ClientActivateAbilityFailed, void);
    DEFINE_FUNC(NetMulticast_InvokeGameplayCueAdded, void);
    DEFINE_FUNC(NetMulticast_InvokeGameplayCueExecuted, void);
    DEFINE_FUNC(MakeEffectContext, FGameplayEffectContextHandle);

    void GiveAbility(const UObject* Ability);
    void GiveAbilitySet(const UFortAbilitySet* Set);
    static void InternalServerTryActivateAbility(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, bool, FPredictionKey*, void*);

    InitHooks;
};