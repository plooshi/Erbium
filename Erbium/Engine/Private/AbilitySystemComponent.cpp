#include "pch.h"
#include "../Public/AbilitySystemComponent.h"
#include "../../Erbium/Public/Finders.h"
#include "../../FortniteGame/Public/FortKismetLibrary.h"

void UAbilitySystemComponent::GiveAbility(const UObject* Ability)
{
    if (!this || !Ability)
        return;

    auto Spec = (FGameplayAbilitySpec*) malloc(FGameplayAbilitySpec::Size());
    __stosb(PBYTE(Spec), 0, FGameplayAbilitySpec::Size());

    static auto ConstructAbilitySpec = FindConstructAbilitySpec();
    if (ConstructAbilitySpec)
        ((void (*)(FGameplayAbilitySpec*, const UObject*, int, int, UObject*)) ConstructAbilitySpec)(Spec, Ability, 1, -1, nullptr);
    else
    {
        Spec->MostRecentArrayReplicationKey = -1;
        Spec->ReplicationID = -1;
        Spec->ReplicationKey = -1;
        Spec->Ability = (UFortGameplayAbility*) Ability;
        Spec->Level = 1;
        Spec->InputID = -1;
        Spec->Handle.Handle = rand();
        if (VersionInfo.FortniteVersion <= 23) Spec->SourceObject = nullptr;
    }

    ((FGameplayAbilitySpecHandle * (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, __int64)) FindGiveAbility())(this, &Spec->Handle, __int64(Spec));
    free(Spec);
}

class IAbilitySystemInterface : public IInterface
{
public:
    UCLASS_COMMON_MEMBERS(IAbilitySystemInterface);
};

void UAbilitySystemComponent::GiveAbilitySet(const UFortAbilitySet* Set)
{
    if (Set)
    {
        for (auto& GameplayAbility : Set->GameplayAbilities)
            GiveAbility(GameplayAbility->GetDefaultObj());
    }
}

struct _Pad_0x10
{
    uint8_t Padding[0x10];
};

struct _Pad_0x18
{
    uint8_t Padding[0x18];
};

void UAbilitySystemComponent::InternalServerTryActivateAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, FPredictionKey* PredictionKey, void* TriggerEventData)
{
    auto Spec = AbilitySystemComponent->ActivatableAbilities.Items.Search([&](FGameplayAbilitySpec& item) {
        return item.Handle.Handle == Handle.Handle;
        }, FGameplayAbilitySpec::Size());

    if (!Spec)
        return AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey->Current);

    Spec->InputPressed |= 1; // it's a bitfield, so we cant just set it to true

    UFortGameplayAbility* InstancedAbility = nullptr;
    auto InternalTryActivateAbility = (bool (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, _Pad_0x10, UFortGameplayAbility**, void*, void*)) FindInternalTryActivateAbility();
    auto InternalTryActivateAbilityNew = (bool (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, _Pad_0x18, UFortGameplayAbility**, void*, void*)) FindInternalTryActivateAbility();
    
    if (!(FPredictionKey::Size() == 0x18 ? InternalTryActivateAbilityNew(AbilitySystemComponent, Handle, *(_Pad_0x18*)PredictionKey, &InstancedAbility, nullptr, TriggerEventData) : InternalTryActivateAbility(AbilitySystemComponent, Handle, *(_Pad_0x10*)PredictionKey, &InstancedAbility, nullptr, TriggerEventData)))
    {
        AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey->Current);
        Spec->InputPressed &= ~1;
        AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
    }
}

void UAbilitySystemComponent::Hook()
{
    uint32 istaIdx = 0;

    if (VersionInfo.EngineVersion > 4.20)
    {
        auto OnRep_ReplicatedAnimMontage = GetDefaultObj()->GetFunction("OnRep_ReplicatedAnimMontage");
        istaIdx = OnRep_ReplicatedAnimMontage->GetVTableIndex() - 1;
    }
    else
    {
        auto ServerTryActivateAbilityWithEventData = GetDefaultObj()->GetFunction("ServerTryActivateAbilityWithEventData");
        auto ServerTryActivateAbilityWithEventDataNativeAddr = __int64(GetDefaultObj()->Vft[ServerTryActivateAbilityWithEventData->GetVTableIndex()]);

        for (int i = 0; i < 400; i++)
        {
            if ((*(uint8_t*)(ServerTryActivateAbilityWithEventDataNativeAddr + i) == 0xFF && *(uint8_t*)(ServerTryActivateAbilityWithEventDataNativeAddr + i + 1) == 0x90) || (*(uint8_t*)(ServerTryActivateAbilityWithEventDataNativeAddr + i) == 0xFF && *(uint8_t*)(ServerTryActivateAbilityWithEventDataNativeAddr + i + 1) == 0x93))
            {
                istaIdx = *(uint32*)(ServerTryActivateAbilityWithEventDataNativeAddr + i + 2) / 8;
                break;
            }
        }
    }

    Utils::HookEvery<UAbilitySystemComponent>(istaIdx, InternalServerTryActivateAbility);
}