#include "pch.h"
#include "../Public/AbilitySystemComponent.h"
#include "../../Erbium/Public/Finders.h"
#include "../../FortniteGame/Public/FortKismetLibrary.h"

uint64 ConstructAbilitySpec;
uint64 GiveAbility_;
FGameplayAbilitySpecHandle UAbilitySystemComponent::GiveAbility(const UObject* Ability, UObject* SourceObject)
{
    if (!this || !Ability)
        return {};
    //printf("GiveAbility[%s]\n", Ability->Name.ToString().c_str());

    auto Spec = (FGameplayAbilitySpec*) malloc(FGameplayAbilitySpec::Size());
    memset(PBYTE(Spec), 0, FGameplayAbilitySpec::Size());

    if (ConstructAbilitySpec)
        ((void (*)(FGameplayAbilitySpec*, const UObject*, int, int, UObject*)) ConstructAbilitySpec)(Spec, Ability, 1, -1, SourceObject);
    else
    {
        Spec->MostRecentArrayReplicationKey = -1;
        Spec->ReplicationID = -1;
        Spec->ReplicationKey = -1;
        Spec->Ability = (UFortGameplayAbility*) Ability;
        Spec->Level = 1;
        Spec->InputID = -1;
        Spec->Handle.Handle = rand();
        Spec->SourceObject = SourceObject;
    }

    FGameplayAbilitySpecHandle OutHandle;
    ((FGameplayAbilitySpecHandle * (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, __int64)) GiveAbility_)(this, &OutHandle, __int64(Spec));
    free(Spec);
    return OutHandle;
}

class IFortAbilitySystemInterface : public IInterface
{
public:
    UCLASS_COMMON_MEMBERS(IFortAbilitySystemInterface);
};

void UAbilitySystemComponent::GiveAbilitySet(const UFortAbilitySet* Set)
{
    TScriptInterface<class IFortAbilitySystemInterface> ScriptInterface;
    ScriptInterface.ObjectPointer = this->GetOwner();
    ScriptInterface.InterfacePointer = ScriptInterface.ObjectPointer->GetInterface(IFortAbilitySystemInterface::StaticClass());

    if (VersionInfo.EngineVersion >= 4.19 && ScriptInterface.ObjectPointer && ScriptInterface.InterfacePointer)
        UFortKismetLibrary::EquipFortAbilitySet(ScriptInterface, Set, nullptr);
    else if (Set)
    {
        //printf("GiveAbilitySet[%s]\n", Set->Name.ToString().c_str());
        for (auto& GameplayAbility : Set->GameplayAbilities)
            GiveAbility(GameplayAbility->GetDefaultObj());
        for (int i = 0; i < Set->PassiveGameplayEffects.Num(); i++)
        {
            auto& GameplayEffect = Set->PassiveGameplayEffects.Get(i, FGameplayEffectApplicationInfo::Size());

            BP_ApplyGameplayEffectToSelf(GameplayEffect.GameplayEffect.Get(), GameplayEffect.Level, MakeEffectContext());
        }
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

uint64_t InternalTryActivateAbility_ = 0;
void UAbilitySystemComponent::InternalServerTryActivateAbility(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, FPredictionKey* PredictionKey, void* TriggerEventData)
{
    auto Spec = AbilitySystemComponent->ActivatableAbilities.Items.Search([&](FGameplayAbilitySpec& item)
    {
        return item.Handle.Handle == Handle.Handle;
        }, FGameplayAbilitySpec::Size());

    if (!Spec)
        return AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey->Current);

    Spec->InputPressed = true;

    UFortGameplayAbility* InstancedAbility = nullptr;
    auto InternalTryActivateAbility = (bool (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, _Pad_0x10, UFortGameplayAbility**, void*, void*)) InternalTryActivateAbility_;
    auto InternalTryActivateAbilityNew = (bool (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, _Pad_0x18, UFortGameplayAbility**, void*, void*)) InternalTryActivateAbility_;
    
    if (!(FPredictionKey::Size() == 0x18 ? InternalTryActivateAbilityNew(AbilitySystemComponent, Handle, *(_Pad_0x18*)PredictionKey, &InstancedAbility, nullptr, TriggerEventData) : InternalTryActivateAbility(AbilitySystemComponent, Handle, *(_Pad_0x10*)PredictionKey, &InstancedAbility, nullptr, TriggerEventData)))
    {
        AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey->Current);
        Spec->InputPressed = false;
        AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
    }
}


void UFortGameplayAbility::K2_AddGameplayCueWithParams_(UObject* Context, FFrame& Stack)
{
    auto& GameplayCueTag = Stack.StepCompiledInRef<FGameplayTag>();
    auto& GameplayCueParameter = Stack.StepCompiledInRef<FGameplayCueParameters>();
    bool bRemoveOnAbilityEnd;

    Stack.StepCompiledIn(&bRemoveOnAbilityEnd);
    Stack.IncrementCode();

    auto Ability = (UFortGameplayAbility*)Context;
    callOG(Ability, Stack.GetCurrentNativeFunction(), K2_AddGameplayCueWithParams, GameplayCueTag, GameplayCueParameter, bRemoveOnAbilityEnd);

    auto PredictionKey = (FPredictionKey*)malloc(FPredictionKey::Size());
    memset((PBYTE)PredictionKey, 0, FPredictionKey::Size());

    auto AbilitySystemComponent = (UAbilitySystemComponent*)Ability->GetAbilitySystemComponentFromActorInfo();

    //AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(GameplayCueTag, *PredictionKey, EffectContext);
    if (AbilitySystemComponent)
        AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded_WithParams(GameplayCueTag, *PredictionKey, GameplayCueParameter);

    free(PredictionKey);
}

void UFortGameplayAbility::K2_ExecuteGameplayCue_(UObject* Context, FFrame& Stack)
{
    auto& GameplayCueTag = Stack.StepCompiledInRef<FGameplayTag>();
    auto& EffectContext = Stack.StepCompiledInRef<FGameplayEffectContextHandle>();
    Stack.IncrementCode();

    auto Ability = (UFortGameplayAbility*)Context;
    callOG(Ability, Stack.GetCurrentNativeFunction(), K2_ExecuteGameplayCue, GameplayCueTag, EffectContext);

    auto PredictionKey = (FPredictionKey*)malloc(FPredictionKey::Size());
    memset((PBYTE)PredictionKey, 0, FPredictionKey::Size());

    auto AbilitySystemComponent = (UAbilitySystemComponent*)Ability->GetAbilitySystemComponentFromActorInfo();

    //AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(GameplayCueTag, *PredictionKey, EffectContext);
    if (AbilitySystemComponent)
        AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(GameplayCueTag, *PredictionKey, EffectContext);

    free(PredictionKey);
}


void UAbilitySystemComponent::Hook()
{
    ConstructAbilitySpec = FindConstructAbilitySpec();
    GiveAbility_ = FindGiveAbility();
    InternalTryActivateAbility_ = FindInternalTryActivateAbility();

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

    //Utils::ExecHook(UFortGameplayAbility::GetDefaultObj()->GetFunction("K2_ExecuteGameplayCue"), UFortGameplayAbility::K2_ExecuteGameplayCue_, UFortGameplayAbility::K2_ExecuteGameplayCue_OG);
    //Utils::ExecHook(UFortGameplayAbility::GetDefaultObj()->GetFunction("K2_AddGameplayCueWithParams"), UFortGameplayAbility::K2_AddGameplayCueWithParams_, UFortGameplayAbility::K2_AddGameplayCueWithParams_OG);
}
