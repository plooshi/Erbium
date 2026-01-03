#include "pch.h"
#include "../Public/FortQuestManager.h"
#include "../Public/FortGameMode.h"
#include <unordered_set>

struct FParseConditionResult
{
    bool bMatch;
    size_t NextStart;
};

static FParseConditionResult ParseCondition(UEAllocatedString Condition, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags)
{
    size_t CondTypeStart = -1, CondTypeEnd = -1, NextCond = -1;

    for (auto& c : Condition)
    {
        if (CondTypeStart == -1)
        {
            bool ud = c == '>' || c == '<' || c == '=' || c == '&' || c == '|';
            if (!ud && strncmp((char*)&c, "HasTag", 6) == 0)
            {
                CondTypeStart = __int64(&c - Condition.data());
                CondTypeEnd = __int64(&c - Condition.data() + 6);
                continue;
            }
            else if (!ud && strncmp((char*)&c, "MissingTag", 10) == 0)
            {
                CondTypeStart = __int64(&c - Condition.data());
                CondTypeEnd = __int64(&c - Condition.data() + 10);
                continue;
            }
            else if (ud)
                CondTypeStart = __int64(&c - Condition.data());
        }
        else if (CondTypeStart != -1 && CondTypeEnd == -1 && (c != '>' && c != '<' && c != '=' && c != '&' && c != '|'))
            CondTypeEnd = __int64(&c - Condition.data());
        else if (CondTypeEnd != -1 && (c == '=' || c == '&' || c == '|'))
            NextCond = __int64(&c - Condition.data());

        if (CondTypeStart != -1 && CondTypeEnd != -1 && NextCond != -1)
            break;
    }

    if (CondTypeStart == Condition.npos)
    {
        CondTypeStart = Condition.find(" ");

        if (CondTypeStart == Condition.npos)
            return { false, NextCond };

        CondTypeStart++;

        if (CondTypeEnd == Condition.npos)
            CondTypeEnd = Condition.find(" ", CondTypeStart);

        NextCond = Condition.find("&&", CondTypeEnd + 1);
        if (NextCond == Condition.npos)
            NextCond = Condition.find("||", CondTypeEnd + 1);
    }
    else if (CondTypeEnd == Condition.npos)
        CondTypeEnd = CondTypeStart + 1;

    auto Left = Condition.substr(0, CondTypeStart - 1);
    auto CondType = Condition.substr(CondTypeStart, CondTypeEnd - CondTypeStart);
    auto Right = Condition.substr(CondTypeEnd + 1, NextCond == Condition.npos ? NextCond : (Condition.substr(NextCond - 1, 1) == " " ? NextCond - 1 : NextCond) - CondTypeEnd - 1);

    if (CondType == "HasTag" || CondType == "MissingTag")
    {
        FGameplayTagContainer Container;

        if (Left == "Target.Tags")
            Container = TargetTags;
        else if (Left == "Source.Tags")
            Container = SourceTags;
        else if (Left == "Context.Tags")
            Container = ContextTags;
        else
            return { false, NextCond };

        FGameplayTag Tag(FName(UEAllocatedWString(Right.begin(), Right.end()).c_str()));

        if (CondType == "HasTag")
            return { Container.HasTag(Tag), NextCond };
        else
            return { !Container.HasTag(Tag), NextCond };
    }
    else
    {
        printf("Condition: %s\n", Condition.c_str());
        printf("Hit unimplemented condition: %s, %s, %s\n", Left.c_str(), CondType.c_str(), Right.c_str());
    }

    return { false, NextCond };
}

static bool IsConditionMet(const FString& InCondition, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags)
{
    if (InCondition.Num() <= 0)
        return true;

    auto Condition = InCondition.ToString();


    FParseConditionResult Result = ParseCondition(Condition, TargetTags, SourceTags, ContextTags);

    if (Result.NextStart != Condition.npos)
    {
    loop:
        if (Result.NextStart == Condition.npos)
            return Result.bMatch;
        auto Start = Condition.substr(Result.NextStart, 2);

        if (Start == "&&")
        {
            Condition = Condition.substr(Result.NextStart + 2);

            if (Condition.substr(0, 1) == " ")
                Condition = Condition.substr(1);

            auto LastResult = Result;

            Result = ParseCondition(Condition, TargetTags, SourceTags, ContextTags);

            if (!LastResult.bMatch || !Result.bMatch)
                Result.bMatch = false;

            goto loop;
        }
        else if (Start == "||")
        {
            Condition = Condition.substr(Result.NextStart + 2);

            if (Condition.substr(0, 1) == " ")
                Condition = Condition.substr(1);

            auto LastResult = Result;

            Result = ParseCondition(Condition, TargetTags, SourceTags, ContextTags);

            if (LastResult.bMatch || Result.bMatch)
                Result.bMatch = true;

            goto loop;
        }
        else
            return Result.bMatch;
    }

    return Result.bMatch;
}

void GiveAccolade(AFortPlayerControllerAthena* PlayerController, UFortAccoladeItemDefinition* Accolade, FPrimaryAssetId AssetId)
{
    printf("GiveAccolade %s\n", Accolade->Name.ToString().c_str());

    auto Info = (FXPEventInfo*)malloc(FXPEventInfo::Size());
    memset(Info, 0, FXPEventInfo::Size());

    Info->Accolade = AssetId;

    float XpValue = Accolade->XpRewardAmount.Evaluate();

    if (XpValue == 0)
        UDataTableFunctionLibrary::EvaluateCurveTableRow(Accolade->XpRewardAmount.Curve.CurveTable, FName(L"Default_Medal"), Accolade->XpRewardAmount.Value, nullptr, &XpValue, FString());

    Info->EventXpValue = (int32)XpValue;
    Info->RestedValuePortion = Info->EventXpValue;
    Info->RestedXPRemaining = Info->EventXpValue;
    Info->TotalXpEarnedInMatch = Info->EventXpValue + PlayerController->XPComponent->TotalXpEarned;
    Info->Priority = Accolade->Priority;
    Info->SimulatedText = Accolade->HasDescription() ? Accolade->Description : Accolade->ItemDescription;
    if (FXPEventInfo::HasSimulatedName())
        Info->SimulatedName = Accolade->HasDisplayName() ? Accolade->DisplayName : Accolade->ItemName;
    Info->EventName = Accolade->Name;
    Info->SeasonBoostValuePortion = 0;
    if (FXPEventInfo::HasPlayerController())
        Info->PlayerController = PlayerController;

    PlayerController->XPComponent->MatchXp += Info->EventXpValue;
    PlayerController->XPComponent->TotalXpEarned += Info->EventXpValue;

    PlayerController->XPComponent->OnXPEvent(*Info);
    free(Info);

    /*for (auto& SoftAccoladeToReplace : Accolade->AccoladeToReplace)
    {
        auto AccoladeToReplace = SoftAccoladeToReplace.Get();
        auto AthenaAccoladeIndex = PlayerController->XPComponent->PlayerAccolades.SearchIndex([&](FAthenaAccolades& item)
            { return item.AccoladeDef == AccoladeToReplace; });

        auto MedalIndex = PlayerController->XPComponent->MedalsEarned.SearchIndex([&](UFortAccoladeItemDefinition* item)
            { return item == AccoladeToReplace; });

        if (AthenaAccoladeIndex != -1)
            PlayerController->XPComponent->PlayerAccolades.Remove(AthenaAccoladeIndex);

        if (MedalIndex != -1)
            PlayerController->XPComponent->MedalsEarned.Remove(MedalIndex);
    }*/


    /*for (auto& AthenaAccolade : PlayerController->XPComponent->PlayerAccolades)
    {
        if (AthenaAccolade.AccoladeDef == Accolade)
        {
            AthenaAccolade.Count++;
            return;
        }
    }

    FAthenaAccolades AthenaAccolade{};
    AthenaAccolade.AccoladeDef = Accolade;
    AthenaAccolade.Count = 1;
    auto str = UEAllocatedWString(L"AthenaAccolade:") + Accolade->Name.ToWString();
    FString TemplateId;
    TemplateId.Reserve((int)str.size() + 1);
    for (auto& c : str)
    {
        TemplateId.Add(c);
    }
    AthenaAccolade.TemplateId = TemplateId;
    PlayerController->XPComponent->PlayerAccolades.Add(AthenaAccolade);*/

    if (Accolade->AccoladeType == 2)
    {
        PlayerController->XPComponent->MedalsEarned.Add(Accolade);
        PlayerController->XPComponent->ClientMedalsRecived(PlayerController->XPComponent->PlayerAccolades);
    }
}

std::unordered_map<UFortQuestManager*, std::unordered_set<UFortQuestItemDefinition*>> OncePerMatchMap;
void ProgressQuest(UFortQuestManager* _this, AFortPlayerControllerAthena* PlayerController, UFortQuestItem* QuestItem, FName BackendName, int Count)
{
    auto QuestDefinition = QuestItem->ItemDefinition;
    auto QuestObjectivePtr = QuestItem->Objectives.Search([&](UFortQuestObjectiveInfo* Obj) { return Obj->BackendName == BackendName; });

    if (!QuestObjectivePtr)
        return; // if this somehow happens, we are just fucked

    if (QuestDefinition->HasbAthenaUpdateObjectiveOncePerMatch() && QuestDefinition->bAthenaUpdateObjectiveOncePerMatch)
    {
        auto& OncePerMatchSet = OncePerMatchMap[_this];
        if (OncePerMatchSet.contains(QuestDefinition))
            return;

        OncePerMatchSet.emplace(QuestDefinition);
    }

    auto QuestObjective = *QuestObjectivePtr;

    auto AcheivedCount = QuestObjective->AchievedCount;

    bool bAllObjectivesCompleted = AcheivedCount + Count == QuestObjective->RequiredCount;

    if (bAllObjectivesCompleted)
    {
        for (auto& Objective : QuestItem->Objectives)
        {
            if (Objective == QuestObjective)
                continue;

            bAllObjectivesCompleted = Objective->AchievedCount == Objective->RequiredCount;

            if (!bAllObjectivesCompleted)
                break;
        }
    }

    printf("[Quests] %s Completed: %s\n", BackendName.ToString().c_str(), (AcheivedCount + Count == QuestObjective->RequiredCount) ? "true" : "false");
    // keep it the same damnit
    _this->SelfCompletedUpdatedQuest(PlayerController, QuestDefinition, BackendName, AcheivedCount + Count, Count, nullptr, AcheivedCount + Count == QuestObjective->RequiredCount, bAllObjectivesCompleted);

    if (!UFortQuestManager::SelfCompletedUpdatedQuest__Ptr)
        _this->HandleQuestUpdated(PlayerController, QuestDefinition, BackendName, AcheivedCount + Count, Count, nullptr, AcheivedCount + Count == QuestObjective->RequiredCount, bAllObjectivesCompleted);

    if (!UFortQuestManager::HandleQuestUpdated__Ptr)
        _this->HandleQuestObjectiveUpdated(PlayerController, QuestDefinition, BackendName, AcheivedCount + Count, Count, nullptr, AcheivedCount + Count == QuestObjective->RequiredCount, bAllObjectivesCompleted);
    if (!UFortQuestManager::HandleQuestObjectiveUpdated__Ptr)
    {
        UFortQuestManagerComponent_Athena* QuestManagerComp = nullptr;

        for (auto& Component : _this->Components)
        {
            if (auto CastedComp = Component->Cast<UFortQuestManagerComponent_Athena>())
            {
                QuestManagerComp = CastedComp;
                break;
            }
        }
        
        if (QuestManagerComp)
            QuestManagerComp->HandleQuestObjectiveUpdated(PlayerController, QuestDefinition, BackendName, AcheivedCount + Count, Count, nullptr, AcheivedCount + Count == QuestObjective->RequiredCount, bAllObjectivesCompleted);
    }
    if (PlayerController->HasXPComponent())
    {
        PlayerController->XPComponent->QuestObjectiveUpdated(PlayerController, QuestDefinition, BackendName, Count, AcheivedCount + Count == QuestObjective->RequiredCount, bAllObjectivesCompleted);
    }
}

std::unordered_map<AActor*, std::unordered_map<int32_t, int32_t>> CountThresholdMap;
std::unordered_set<UFortAccoladeItemDefinition*> OnlyOnceMap;

void UFortQuestManager::SendStatEvent__Internal(AActor* PlayerController, long long StatEvent, int32 Count, UObject* TargetObject, FGameplayTagContainer TargetTags, FGameplayTagContainer SourceTags, FGameplayTagContainer ContextTags, bool* QuestActive, bool* QuestCompleted)
{
    if (!this)
        return;

	static auto XPTable = FindObject<UDataTable>(L"/Game/Athena/Items/Quests/AthenaObjectiveStatXPTable.AthenaObjectiveStatXPTable");
    auto FortPC = (AFortPlayerControllerAthena*)PlayerController;

	if (XPTable && FortPC->HasXPComponent())
	{
		for (const auto& [ Key, Value ] : XPTable->RowMap)
		{
			auto Row = (FFortQuestObjectiveStatXPTableRow*)Value;

			if (Row->Type != (uint8_t)StatEvent) // todo: fix this count threshold stuff
				continue;

			if (!Row->HasAccoladePrimaryAssetId())
				break;
			
            auto Accolade = (UFortAccoladeItemDefinition*)UKismetSystemLibrary::GetObjectFromPrimaryAssetId(Row->AccoladePrimaryAssetId);

            if (!Accolade)
                continue;

            if (Row->bOnceOnly && OnlyOnceMap.contains(Accolade))
                continue;

			if (!TargetTags.HasAll(Row->TargetTags))
				continue;

			if (!SourceTags.HasAll(Row->SourceTags))
				continue;

			if (!ContextTags.HasAll(Row->ContextTags))
				continue;

			if (FFortQuestObjectiveStatXPTableRow::HasExcludeTargetTags() && TargetTags.HasAny(Row->ExcludeTargetTags))
				continue;

			if (FFortQuestObjectiveStatXPTableRow::HasExcludeSourceTags() && SourceTags.HasAny(Row->ExcludeSourceTags))
				continue;

			if (FFortQuestObjectiveStatXPTableRow::HasExcludeContextTags() && ContextTags.HasAny(Row->ExcludeContextTags))
				continue;

            if (!IsConditionMet(Row->Condition, TargetTags, SourceTags, ContextTags))
                continue;

            if (PlayerController)
            {
                auto& PrimaryAssetName = *(int32*)(__int64(&Row->AccoladePrimaryAssetId) + (VersionInfo.FortniteVersion >= 20 ? 4 : 8));
                if (PlayerController)
                    CountThresholdMap[PlayerController][PrimaryAssetName] += Count;

                if (Row->CountThreshhold > 0 && CountThresholdMap[PlayerController][PrimaryAssetName] != Row->CountThreshhold)
                    continue;

                auto AccoladeCount = 0;
                for (auto& AthenaAccolade : FortPC->XPComponent->PlayerAccolades)
                {
                    if (AthenaAccolade.AccoladeDef == Accolade)
                    {
                        AccoladeCount = AthenaAccolade.Count;
                        break;
                    }
                }

                printf("%s %d\n", Accolade->Name.ToString().c_str(), AccoladeCount);
                if (AccoladeCount > Row->MaxCount)
                    continue;

                if (Row->bOnceOnly)
                    OnlyOnceMap.emplace(Accolade);

                GiveAccolade(FortPC, Accolade, Row->AccoladePrimaryAssetId);
            }
		}
	}

    for (auto& Quest : CurrentQuests)
    {
        if (Quest->HasCompletedQuest())
            continue;

        auto QuestDefinition = Quest->ItemDefinition;


        for (int i = 0; i < QuestDefinition->Objectives.Num(); i++)
        {
            auto& Objective = QuestDefinition->Objectives.Get(i, FFortMcpQuestObjectiveInfo::Size());

            if (Quest->HasCompletedObjectiveWithName(Objective.BackendName))
                continue;

            if (Objective.ObjectiveStatHandle.RowName.IsValid() && Objective.ObjectiveStatHandle.DataTable)
            {
                for (auto& [Key, Value] : Objective.ObjectiveStatHandle.DataTable->RowMap)
                {
                    if (Key != Objective.ObjectiveStatHandle.RowName)
                        continue;

                    auto Row = (FFortQuestObjectiveStatTableRow*)Value;

                    if (Row->Type != StatEvent)
                        continue;

                    if (!TargetTags.HasAll(Row->TargetTagContainer))
                        continue;

                    if (!SourceTags.HasAll(Row->SourceTagContainer))
                        continue;

                    if (!ContextTags.HasAll(Row->ContextTagContainer))
                        continue;

                    if (!IsConditionMet(Row->Condition, TargetTags, SourceTags, ContextTags))
                        continue;

                    printf("[Quests] Update: %s\n", Objective.BackendName.ToString().c_str());
                    ProgressQuest(this, FortPC, Quest, Objective.BackendName, Count);
                }
            }
            else
            {
                if (FFortMcpQuestObjectiveInfo::HasInlineObjectiveStats())
                {
                    for (int i = 0; i < Objective.InlineObjectiveStats.Num(); i++)
                    {
                        auto& ObjectiveStat = Objective.InlineObjectiveStats.Get(i, FFortQuestObjectiveStat::Size());

                        if (ObjectiveStat.Type != StatEvent)
                            continue;

                        bool bFound = true;
                        for (int i = 0; i < ObjectiveStat.TagConditions.Num(); i++)
                        {
                            auto& Condition = ObjectiveStat.TagConditions.Get(i, FInlineObjectiveStatTagCheckEntry::Size());

                            auto& Tag = FInlineObjectiveStatTagCheckEntry::HasTag() ? Condition.Tag : Condition.tag;

                            bool bHasTag = false;
                            switch (Condition.Type)
                            {
                            case Target:
                                bHasTag = TargetTags.HasTag(Tag);
                                break;
                            case Source:
                                bHasTag = SourceTags.HasTag(Tag);
                                break;
                            case Context:
                                bHasTag = ContextTags.HasTag(Tag);
                                break;
                            }

                            if (Condition.Require ? !bHasTag : bHasTag)
                            {
                                bFound = false;
                                break;
                            }
                        }

                        if (!bFound)
                            continue;

                        if (!IsConditionMet(ObjectiveStat.Condition, TargetTags, SourceTags, ContextTags))
                            continue;

                        ProgressQuest(this, FortPC, Quest, Objective.BackendName, Count);
                    }
                }
            }
        }
    }
}

bool bHasQueueStatEvent = false;

void UFortQuestManager::SendStatEvent(AActor* PlayerController, long long StatEvent, int32 Count, bool bAllowQueueStatEvent, UObject* TargetObject, FGameplayTagContainer TargetTags, FGameplayTagContainer AdditionalSourceTags, bool* QuestActive, bool* QuestCompleted)
{
    if ((bHasQueueStatEvent && !bAllowQueueStatEvent) || !this)
        return;

    FGameplayTagContainer PlayerSourceTags;
    FGameplayTagContainer ContextTags;

    GetSourceAndContextTags(&PlayerSourceTags, &ContextTags);

    printf("[QuestManager] SendStatEvent (Event: %lld)\n", StatEvent);

    auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    auto Playlist = VersionInfo.FortniteVersion >= 3.5 && GameMode->HasWarmupRequiredPlayerCount() ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData) : nullptr;

    if (Playlist && Playlist->HasGameplayTagContainer())
        ContextTags.AppendTags(Playlist->GameplayTagContainer);
    AdditionalSourceTags.AppendTags(PlayerSourceTags);

    return SendStatEvent__Internal(PlayerController, StatEvent, Count, TargetObject, TargetTags, AdditionalSourceTags, ContextTags, QuestActive, QuestCompleted);
}

bool bHasQuestActive = false;
bool bHasQuestCompleted = false;
void SendComplexCustomStatEvent(UObject* Context, FFrame& Stack)
{
    UObject* TargetObject;
    FGameplayTagContainer AdditionalSourceTags;
    FGameplayTagContainer TargetTags;
    bool* QuestActive = nullptr;
    bool* QuestCompleted = nullptr;
    int32 Count;

    Stack.StepCompiledIn(&TargetObject);
    Stack.StepCompiledIn(&AdditionalSourceTags);
    Stack.StepCompiledIn(&TargetTags);
    if (bHasQuestActive)
        Stack.StepCompiledIn(&QuestActive);
    if (bHasQuestCompleted)
        Stack.StepCompiledIn(&QuestCompleted);
    Stack.StepCompiledIn(&Count);
    Stack.IncrementCode();
    auto QuestManager = (UFortQuestManager*)Context;

    auto PlayerController = (AFortPlayerControllerAthena*)QuestManager->GetPlayerControllerBP();

    printf("SendComplexCustomStatEvent\n");
    if (VersionInfo.FortniteVersion != 11.31) // js crashes idk why
        QuestManager->SendStatEvent(PlayerController, EFortQuestObjectiveStatEvent::GetComplexCustom(), Count, false, TargetObject, TargetTags, AdditionalSourceTags);
}

void QueueStatEvent(UFortQuestManager* QuestManager, uint8_t InType, UObject* InTargetObject, FGameplayTagContainer* InTargetTags, FGameplayTagContainer* InSourceTags, FGameplayTagContainer* InContextTags, void* InObjectiveStat, FName InObjectiveBackendName, int InCount)
{
    printf("[QuestManager] QueueStatEvent (Event: %d)\n", InType);

    auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;

    auto Playlist = VersionInfo.FortniteVersion >= 3.5 && GameMode->HasWarmupRequiredPlayerCount() ? (GameMode->GameState->HasCurrentPlaylistInfo() ? GameMode->GameState->CurrentPlaylistInfo.BasePlaylist : GameMode->GameState->CurrentPlaylistData) : nullptr;

    if (Playlist && Playlist->HasGameplayTagContainer())
        (*InContextTags).AppendTags(Playlist->GameplayTagContainer);

    return QuestManager->SendStatEvent__Internal(QuestManager->GetPlayerControllerBP(), InType, InCount, InTargetObject, *InTargetTags, *InSourceTags, *InContextTags, nullptr, nullptr);
}

void UFortQuestManager::PostLoadHook()
{

    auto QueueStatEventAddr = FindQueueStatEvent();

    bHasQueueStatEvent = QueueStatEventAddr != 0;
    Utils::Hook(QueueStatEventAddr, QueueStatEvent);

    if (!bHasQueueStatEvent)
    {
        auto SendComplexCustomStatEventFn = GetDefaultObj()->GetFunction("SendComplexCustomStatEvent");

        if (SendComplexCustomStatEventFn)
            for (auto& Param : SendComplexCustomStatEventFn->GetParamsNamed().NameOffsetMap)
            {
                if (Param.Name == "QuestActive")
                    bHasQuestActive = true;
                else if (Param.Name == "QuestCompleted")
                    bHasQuestCompleted = true;
            }

        Utils::ExecHook(SendComplexCustomStatEventFn, SendComplexCustomStatEvent);
    }
}
