#include "pch.h"
#include "../Public/FortQuestManager.h"
#include "../Public/FortGameMode.h"

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

std::unordered_map<AActor*, std::unordered_map<int32_t, int32_t>> CountThresholdMap;
std::set<UFortAccoladeItemDefinition*> OnlyOnceMap;

void UFortQuestManager::SendStatEvent(AActor* PlayerController, long long StatEvent, int32 Count, UObject* TargetObject, FGameplayTagContainer TargetTags, FGameplayTagContainer AdditionalSourceTags, bool* QuestActive, bool* QuestCompleted)
{
    if (!this)
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

			if (!AdditionalSourceTags.HasAll(Row->SourceTags))
				continue;

			if (!ContextTags.HasAll(Row->ContextTags))
				continue;

			if (FFortQuestObjectiveStatXPTableRow::HasExcludeTargetTags() && TargetTags.HasAny(Row->ExcludeTargetTags))
				continue;

			if (FFortQuestObjectiveStatXPTableRow::HasExcludeSourceTags() && AdditionalSourceTags.HasAny(Row->ExcludeSourceTags))
				continue;

			if (FFortQuestObjectiveStatXPTableRow::HasExcludeContextTags() && ContextTags.HasAny(Row->ExcludeContextTags))
				continue;

            if (!IsConditionMet(Row->Condition, TargetTags, AdditionalSourceTags, ContextTags))
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


    }
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
    QuestManager->SendStatEvent(PlayerController, EFortQuestObjectiveStatEvent::GetComplexCustom(), Count, TargetObject, TargetTags, AdditionalSourceTags);
}


void UFortQuestManager::PostLoadHook()
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
