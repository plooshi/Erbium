#pragma once
#include "../../pch.h"
#include "GameplayTagContainer.h"
#include "FortInventory.h"

struct FFortQuestObjectiveStatXPTableRow
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortQuestObjectiveStatXPTableRow);

    DEFINE_STRUCT_PROP(Type, uint8);
    DEFINE_STRUCT_PROP(TargetTags, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(SourceTags, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(ContextTags, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(ExcludeTargetTags, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(ExcludeSourceTags, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(ExcludeContextTags, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(CountThreshhold, int32);
    DEFINE_STRUCT_PROP(MaxCount, int32);
    DEFINE_STRUCT_PROP(Condition, FString);
    DEFINE_STRUCT_PROP(AccoladePrimaryAssetId, FPrimaryAssetId);
    DEFINE_STRUCT_PROP(bOnceOnly, bool);
};

class EFortQuestObjectiveStatEvent
{
public:
    UENUM_COMMON_MEMBERS(EFortQuestObjectiveStatEvent);

    DEFINE_ENUM_PROP(StormPhase);
    DEFINE_ENUM_PROP(Interact);
    DEFINE_ENUM_PROP(Kill);
    DEFINE_ENUM_PROP(Win);
    DEFINE_ENUM_PROP(KillContribution);
    DEFINE_ENUM_PROP(ComplexCustom);
    DEFINE_ENUM_PROP(Emote);
    DEFINE_ENUM_PROP(Spray);
    DEFINE_ENUM_PROP(Toy);
    DEFINE_ENUM_PROP(Land);
    DEFINE_ENUM_PROP(Damage);
};

class IGameplayTagAssetInterface : public IInterface
{
public:
    UCLASS_COMMON_MEMBERS(IGameplayTagAssetInterface);

    DEFINE_FUNC(GetOwnedGameplayTags, void);
};

class UFortAccoladeItemDefinition : public UFortItemDefinition
{
public:
    UCLASS_COMMON_MEMBERS(UFortAccoladeItemDefinition);

    DEFINE_PROP(XpRewardAmount, FScalableFloat);
    DEFINE_PROP(Priority, uint8);
    DEFINE_PROP(AccoladeType, uint8);
    DEFINE_PROP(AccoladeToReplace, TArray<TSoftObjectPtr<UFortAccoladeItemDefinition>>);
};

struct FXPEventInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FXPEventInfo);

    DEFINE_STRUCT_PROP(Accolade, FPrimaryAssetId);
    DEFINE_STRUCT_PROP(EventXpValue, int32);
    DEFINE_STRUCT_PROP(RestedValuePortion, int32);
    DEFINE_STRUCT_PROP(RestedXPRemaining, int32);
    DEFINE_STRUCT_PROP(TotalXpEarnedInMatch, int32);
    DEFINE_STRUCT_PROP(Priority, uint8);
    DEFINE_STRUCT_PROP(SimulatedText, FText);
    DEFINE_STRUCT_PROP(SimulatedName, FText);
    DEFINE_STRUCT_PROP(EventName, FName);
    DEFINE_STRUCT_PROP(SeasonBoostValuePortion, int32);
    DEFINE_STRUCT_PROP(PlayerController, AActor*);
};

struct FAthenaAccolades
{
public:
    UFortAccoladeItemDefinition* AccoladeDef;
    FString TemplateId;
    int32 Count;
};

class UFortQuestItemDefinition : public UFortItem
{
public:
    UCLASS_COMMON_MEMBERS(UFortQuestItemDefinition);
};

class UFortQuestItem : public UFortItem
{
public:
    UCLASS_COMMON_MEMBERS(UFortQuestItem);

    DEFINE_PROP(ItemDefinition, UFortQuestItemDefinition*);

    DEFINE_FUNC(HasCompletedQuest, bool);
};


class UFortQuestManager : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortQuestManager);

    DEFINE_PROP(CurrentQuests, TArray<UFortQuestItem*>);

    DEFINE_FUNC(GetSourceAndContextTags, void);
    DEFINE_FUNC(GetPlayerControllerBP, AActor*);

    void SendStatEvent(AActor* PlayerController, long long StatEvent, int32 Count, UObject* TargetObject = nullptr, FGameplayTagContainer TargetTags = FGameplayTagContainer(), FGameplayTagContainer AdditionalSourceTags = FGameplayTagContainer(), bool* QuestActive = nullptr, bool* QuestCompleted = nullptr);


    InitPostLoadHooks;
};