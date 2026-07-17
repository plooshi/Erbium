#include "pch.h"
#include "../Public/FortWeaponRanged.h"

void AFortWeaponRanged::ServerNotifyPawnHit(UObject* Context, FFrame& Stack)
{
    AFortWeaponRanged* Weapon = (AFortWeaponRanged*)Context;
    FHitResult& Hit = Stack.StepCompiledInRef<FHitResult>();
    FVector& ProjectileOriginPosition = Stack.StepCompiledInRef<FVector>();
    Stack.IncrementCode();
    auto Instigator = Weapon->GetInstigator();
    if (!Instigator)
        return;

    auto ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);

    if (!ASC)
        return;

    auto Spec = ASC->ActivatableAbilities.Items.Search([Weapon](FGameplayAbilitySpec& item) { return item.Handle.Handle == Weapon->PrimaryAbilitySpecHandle.Handle; });

    auto FireAbility = (UFortGameplayAbility*)Spec->Ability;

    auto Target = Hit.Component.Get()->GetOwner();

    if (!FireAbility || !Target)
        return;

    for (auto i = 0; i < 5; i++)
    {
        auto Container = FireAbility->EffectContainers[i];
        for (auto GE : Container.TargetGameplayEffectClasses)
        {
            auto ContextHandle = ASC->MakeEffectContext();
            auto Context = *(FFortGameplayEffectContext**)((__int64(&ContextHandle) + 0x8));
            UAbilitySystemBlueprintLibrary::EffectContextAddHitResult<FGameplayEffectContextHandle&, FHitResult&, bool>(ContextHandle, Hit, true);

            Context->EffectCauser = Weapon;
            Context->Instigator = Instigator;
            Context->InstigatorAbilitySystemComponent = ASC;

            Context->DamageSourceObject = Weapon;

            Context->WorldOrigin = ProjectileOriginPosition;

            auto SpecHandle = ASC->MakeOutgoingSpec(GE, 1.f, ContextHandle);

            auto TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
            ASC->BP_ApplyGameplayEffectSpecToTarget(SpecHandle, TargetASC);
        }
    }

}

void AFortWeaponRanged::PostLoadHook()
{
    Hooking::ExecHook(GetDefaultObj()->GetFunction("ServerNotifyPawnHit"), ServerNotifyPawnHit);
}
