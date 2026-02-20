#pragma once
#include "../../pch.h"
#include "FortInventory.h"
#include "GameplayTagContainer.h"

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
    DEFINE_ENUM_PROP(EarnVehicleTrickPoints);
    DEFINE_ENUM_PROP(VehicleAirTime);
    DEFINE_ENUM_PROP(Visit);
    DEFINE_ENUM_PROP(Build);
    DEFINE_ENUM_PROP(Collect);
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

struct FFortQuestObjectiveStatTableRow
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortQuestObjectiveStatTableRow);

    DEFINE_STRUCT_PROP(Type, uint8);
    DEFINE_STRUCT_PROP(TargetTagContainer, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(SourceTagContainer, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(ContextTagContainer, FGameplayTagContainer);
    DEFINE_STRUCT_PROP(Condition, FString);
};

enum EInlineObjectiveStatTagCheckEntryType : uint8
{
    Target = 0,
    Source = 1,
    Context = 2
};

struct FInlineObjectiveStatTagCheckEntry
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FInlineObjectiveStatTagCheckEntry);

    DEFINE_STRUCT_PROP(Tag, FGameplayTag);
    DEFINE_STRUCT_PROP(tag, FGameplayTag);
    DEFINE_STRUCT_PROP(Type, EInlineObjectiveStatTagCheckEntryType);
    DEFINE_STRUCT_BITFIELD_PROP(Require);
};

struct FFortQuestObjectiveStat
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortQuestObjectiveStat);

    DEFINE_STRUCT_PROP(Type, uint8);
    DEFINE_STRUCT_PROP(Condition, FString);
    DEFINE_STRUCT_PROP(TagConditions, TArray<FInlineObjectiveStatTagCheckEntry>);
};

struct FFortMcpQuestObjectiveInfo
{
public:
    USCRIPTSTRUCT_COMMON_MEMBERS(FFortMcpQuestObjectiveInfo);

    DEFINE_STRUCT_PROP(BackendName, FName);
    DEFINE_STRUCT_PROP(ObjectiveStatHandle, FDataTableRowHandle);
    DEFINE_STRUCT_PROP(InlineObjectiveStats, TArray<FFortQuestObjectiveStat>);
};

class UFortQuestItemDefinition : public UFortItem
{
public:
    UCLASS_COMMON_MEMBERS(UFortQuestItemDefinition);

    DEFINE_PROP(Objectives, TArray<FFortMcpQuestObjectiveInfo>);
    DEFINE_BITFIELD_PROP(bAthenaUpdateObjectiveOncePerMatch);
    DEFINE_BITFIELD_PROP(bAthenaMustCompleteInSingleMatch);
};

class UFortQuestObjectiveInfo : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortQuestObjectiveInfo);

    DEFINE_PROP(BackendName, FName);
    DEFINE_PROP(AchievedCount, int32);
    DEFINE_PROP(RequiredCount, int32);
};

class UFortQuestItem : public UFortItem
{
public:
    UCLASS_COMMON_MEMBERS(UFortQuestItem);

    DEFINE_PROP(ItemDefinition, UFortQuestItemDefinition*);
    DEFINE_PROP(Objectives, TArray<UFortQuestObjectiveInfo*>);

    DEFINE_FUNC(HasCompletedQuest, bool);
    DEFINE_FUNC(HasCompletedObjectiveWithName, bool);
};

class UFortQuestManagerComponent_Athena : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortQuestManagerComponent_Athena);

    DEFINE_FUNC(HandleQuestObjectiveUpdated, void);
};

class UFortQuestManager : public UObject
{
public:
    UCLASS_COMMON_MEMBERS(UFortQuestManager);

    DEFINE_PROP(CurrentQuests, TArray<UFortQuestItem*>);
    DEFINE_PROP(Components, TArray<UObject*>);

    DEFINE_FUNC(GetSourceAndContextTags, void);
    DEFINE_FUNC(GetPlayerControllerBP, AActor*);
    DEFINE_FUNC(SelfCompletedUpdatedQuest, void);
    DEFINE_FUNC(HandleQuestUpdated, void);
    DEFINE_FUNC(HandleQuestObjectiveUpdated, void);

    void SendStatEvent__Internal(AActor* PlayerController, long long StatEvent, int32 Count, UObject* TargetObject, FGameplayTagContainer TargetTags,
        FGameplayTagContainer SourceTags, FGameplayTagContainer ContextTags, bool* QuestActive, bool* QuestCompleted);
    void SendStatEvent(AActor* PlayerController, long long StatEvent, int32 Count, bool bAllowQueueStatEvent, UObject* TargetObject = nullptr,
        FGameplayTagContainer TargetTags = FGameplayTagContainer(), FGameplayTagContainer AdditionalSourceTags = FGameplayTagContainer(), bool* QuestActive = nullptr,
        bool* QuestCompleted = nullptr);

    InitPostLoadHooks;
};