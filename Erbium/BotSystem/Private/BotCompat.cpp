#include "pch.h"
#include "../Public/BotCompat.h"
#include "../Public/BotEngineTypes.h"
#include "../Public/BotLog.h"
#include "../../FortniteGame/Public/FortGameMode.h"
#include "../../FortniteGame/Public/FortKismetLibrary.h"

// Engine functions Erbium locates at startup (see Finders.cpp / FortGameMode.cpp /
// FortPlayerControllerAthena.cpp). Any of them may be 0 on a build where the pattern was not found.
extern uint64_t ApplyCharacterCustomization;
extern uint64_t NotifyGameMemberAdded_;
extern uint64 RemoveFromAlivePlayers_;

namespace BotCompat
{
    static bool IsChildOf(const UStruct* Struct, const UStruct* Parent)
    {
        if (!Parent)
            return false;

        for (; Struct; Struct = Struct->GetSuper())
            if (Struct == Parent)
                return true;

        return false;
    }

    static void AddPlayerControllerCandidates(AFortGameMode* GameMode, std::vector<FControllerClassChoice>& Out)
    {
        if (GameMode->HasPlayerControllerClass())
        {
            auto Class = GameMode->PlayerControllerClass;

            if (Class && IsChildOf(Class, AFortPlayerControllerAthena::StaticClass()))
                Out.push_back({ Class, EBotControllerKind::PlayerController });
        }

        static auto Fallback = FindObject<UClass>(L"/Game/Athena/Athena_PlayerController.Athena_PlayerController_C");
        if (Fallback)
            Out.push_back({ Fallback, EBotControllerKind::PlayerController });
    }

    static void AddAIControllerCandidates(std::vector<FControllerClassChoice>& Out)
    {
        // Plain engine AI controller first: it has no Fortnite bot logic of its own that could need
        // bot data we do not provide, or compete with the bot system for the pawn.
        if (auto EngineAIController = AAIController::StaticClass())
            Out.push_back({ EngineAIController, EBotControllerKind::AIController });

        // Fortnite's own bot controller (the class Epic's server bots use in this version range).
        static auto FortBotController = FindClass("FortAthenaAIBotController");
        if (FortBotController)
            Out.push_back({ FortBotController, EBotControllerKind::AIController });
    }

    std::vector<FControllerClassChoice> GetBotControllerCandidates(AFortGameMode* GameMode)
    {
        std::vector<FControllerClassChoice> Candidates;

        switch (FBotSystemConfig::ControllerMode)
        {
        case EBotControllerMode::AIController:
            AddAIControllerCandidates(Candidates);
            break;
        case EBotControllerMode::PlayerController:
            AddPlayerControllerCandidates(GameMode, Candidates);
            break;
        case EBotControllerMode::Auto:
            AddAIControllerCandidates(Candidates);
            AddPlayerControllerCandidates(GameMode, Candidates);
            break;
        }

        if (Candidates.empty())
            BOT_LOG(BOT_CATEGORY_COMPAT, "No usable controller class found on this build.");

        return Candidates;
    }

    const UClass* GetBotPawnClass(AFortGameMode* GameMode, AController* Controller)
    {
        if (auto Class = GameMode->GetDefaultPawnClassForController(Controller))
            return Class;

        return GameMode->DefaultPawnClass;
    }

    AFortPlayerStateAthena* EnsurePlayerState(AController* Controller, const FTransform& Transform)
    {
        if (auto Existing = Controller->PlayerState)
            return Existing;

        BOT_VERBOSE(BOT_CATEGORY_COMPAT, "Controller has no player state after spawn, creating one.");

        auto PlayerState = (AFortPlayerStateAthena*)UWorld::SpawnActor(AFortPlayerStateAthena::StaticClass(), Transform);
        if (!PlayerState)
            return nullptr;

        PlayerState->SetOwner(Controller);
        Controller->PlayerState = PlayerState;
        Controller->OnRep_PlayerState();

        return PlayerState;
    }

    AFortInventory* EnsureWorldInventory(AController* Controller, EBotControllerKind Kind)
    {
        if (Kind != EBotControllerKind::PlayerController)
            return nullptr;

        auto PlayerController = (AFortPlayerControllerAthena*)Controller;

        if (auto Existing = PlayerController->WorldInventory)
            return Existing;

        if (!PlayerController->HasWorldInventoryClass() || !PlayerController->WorldInventoryClass)
            return nullptr;

        auto Inventory = UWorld::SpawnActor<AFortInventory>(PlayerController->WorldInventoryClass, FVector{}, FRotator{}, PlayerController);
        if (!Inventory)
            return nullptr;

        Inventory->InventoryType = 0;
        PlayerController->WorldInventory = Inventory;

        return Inventory;
    }

    void DisableBuiltInAI(AController* Controller, EBotControllerKind Kind)
    {
        if (Kind != EBotControllerKind::AIController)
            return;

        auto AIController = (AAIController*)Controller;
        if (!AIController->HasBrainComponent())
            return;

        if (auto Brain = AIController->BrainComponent)
        {
            FString Reason = FString(L"Erbium bot system");
            Brain->StopLogic(Reason);
        }
    }

    bool PossessPawn(AController* Controller, EBotControllerKind Kind, AFortPlayerPawnAthena* Pawn, AFortPlayerStateAthena* PlayerState)
    {
        Controller->Possess(Pawn);

        if (Controller->Pawn != Pawn || Pawn->Controller != Controller)
            return false;

        if (Kind == EBotControllerKind::PlayerController)
        {
            // The original spawnbot notes this crashes on 27+ when left unset.
            auto PlayerController = (AFortPlayerControllerAthena*)Controller;
            if (PlayerController->HasMyFortPawn())
                PlayerController->MyFortPawn = Pawn;
        }

        if (PlayerState && Pawn->PlayerState != PlayerState)
        {
            Pawn->PlayerState = PlayerState;
            Pawn->OnRep_PlayerState();
        }

        return true;
    }

    void AssignTeam(AFortGameMode* GameMode, AController* Controller, AFortPlayerStateAthena* PlayerState)
    {
        auto GameState = GameMode->GameState;

        // PickTeam only reads the game mode / playlist; the controller argument is unused.
        PlayerState->TeamIndex = AFortGameMode::PickTeam(GameMode, 0, (AFortPlayerControllerAthena*)Controller);

        if (PlayerState->HasSquadId())
        {
            PlayerState->SquadId = PlayerState->TeamIndex - 3;
            PlayerState->OnRep_SquadId();
        }

        if (PlayerState->HasbIsABot())
            PlayerState->bIsABot = true;

        if (!GameState->HasGameMemberInfoArray())
            return;

        auto Member = (FGameMemberInfo*)malloc(FGameMemberInfo::Size());
        memset((PBYTE)Member, 0, FGameMemberInfo::Size());

        Member->MostRecentArrayReplicationKey = -1;
        Member->ReplicationID = -1;
        Member->ReplicationKey = -1;
        Member->TeamIndex = PlayerState->TeamIndex;
        Member->SquadId = PlayerState->SquadId;
        Member->MemberUniqueId = PlayerState->HasUniqueID() ? PlayerState->UniqueID : PlayerState->UniqueId;

        auto& NewMember = GameState->GameMemberInfoArray.Members.Add(*Member, FGameMemberInfo::Size());
        GameState->GameMemberInfoArray.MarkItemDirty(NewMember);

        auto NotifyGameMemberAdded = (void (*)(AFortGameStateAthena*, uint8_t, uint8_t, FUniqueNetIdRepl*))NotifyGameMemberAdded_;
        if (NotifyGameMemberAdded)
            NotifyGameMemberAdded(GameState, Member->SquadId, Member->TeamIndex, &Member->MemberUniqueId);

        free(Member);
    }

    void SetPlayerName(AFortGameMode* GameMode, AController* Controller, EBotControllerKind Kind, AFortPlayerStateAthena* PlayerState, const wchar_t* Name)
    {
        FString BotName = FString(Name);

        if (Kind == EBotControllerKind::PlayerController && std::floor(VersionInfo.FortniteVersion) < 9)
            ((AFortPlayerControllerAthena*)Controller)->ServerChangeName(BotName);
        else
            GameMode->ChangeName(Controller, BotName, true);

        PlayerState->OnRep_PlayerName();
    }

    void ApplyDefaultAppearance(AFortPlayerStateAthena* PlayerState, AFortPlayerPawnAthena* Pawn)
    {
        static auto Commando = FindObject(L"/Game/Athena/Heroes/HID_001_Athena_Commando_F.HID_001_Athena_Commando_F", nullptr);
        static auto Commando2 = FindObject(L"/Game/Athena/Heroes/HID_Commando_Athena_01.HID_Commando_Athena_01", nullptr);
        if (PlayerState->HasHeroType())
            PlayerState->HeroType = Commando ? Commando : Commando2;

        static auto Head = FindObject<UObject>(L"/Game/Characters/CharacterParts/Female/Medium/Heads/F_Med_Head1.F_Med_Head1");
        static auto Body = FindObject<UObject>(L"/Game/Characters/CharacterParts/Female/Medium/Bodies/F_Med_Soldier_01.F_Med_Soldier_01");
        static auto Backpack = FindObject<UObject>(L"/Game/Characters/CharacterParts/Backpacks/NoBackpack.NoBackpack");

        // Layout 1: CharacterParts is an FCustomCharacterParts struct (0x100000 = StructProperty cast flag).
        // Layout 2: CharacterParts/LocalCharacterParts is a raw fixed-size array of part pointers.
        static auto StructPartsOffset = PlayerState->GetOffset("CharacterParts", 0x100000);
        const UObject** CharacterParts = nullptr;

        if (StructPartsOffset != -1)
        {
            static auto CustomCharacterPartsStruct = SDK::FindStruct("CustomCharacterParts");
            static auto PartsOffset = CustomCharacterPartsStruct ? CustomCharacterPartsStruct->GetOffset("Parts") : -1;

            if (PartsOffset != -1)
                CharacterParts = (const UObject**)(__int64(PlayerState) + StructPartsOffset + PartsOffset);
        }
        else
        {
            static auto RawPartsOffset = PlayerState->GetOffset("CharacterParts");
            static auto LocalPartsOffset = PlayerState->GetOffset("LocalCharacterParts");
            auto Offset = RawPartsOffset != -1 ? RawPartsOffset : LocalPartsOffset;

            if (Offset != -1)
                CharacterParts = (const UObject**)(__int64(PlayerState) + Offset);
        }

        if (CharacterParts)
        {
            // Indices follow EFortCustomPartType: 0 = Head, 1 = Body, 3 = Backpack.
            CharacterParts[0] = Head;
            CharacterParts[1] = Body;
            CharacterParts[3] = Backpack;
        }
        else
            BOT_LOG(BOT_CATEGORY_COMPAT, "Could not locate CharacterParts on this build; bot will use engine default appearance.");

        // Same visual refresh ServerAcknowledgePossession uses for real players.
        UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
        if (!UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization__Ptr && ApplyCharacterCustomization)
            ((void (*)(AActor*, AActor*))ApplyCharacterCustomization)(PlayerState, Pawn);
    }

    static int32 FindAlivePlayerIndex(AFortGameMode* GameMode, AController* Controller)
    {
        if (!GameMode->HasAlivePlayers())
            return -1;

        auto& AlivePlayers = GameMode->AlivePlayers;
        for (int32 i = 0; i < AlivePlayers.Num(); i++)
        {
            if (AlivePlayers[i] == Controller)
                return i;
        }

        return -1;
    }

    static void ChangePlayersLeft(AFortGameMode* GameMode, int Delta)
    {
        auto GameState = GameMode->GameState;
        if (!GameState->HasPlayersLeft())
            return;

        if (Delta < 0 && GameState->PlayersLeft <= 0)
            return;

        GameState->PlayersLeft += Delta;
        GameState->OnRep_PlayersLeft();
    }

    void RegisterAlive(AFortGameMode* GameMode, AController* Controller, EBotControllerKind Kind)
    {
        if (Kind == EBotControllerKind::AIController)
        {
            ChangePlayersLeft(GameMode, 1);
            return;
        }

        if (!GameMode->HasAlivePlayers() || FindAlivePlayerIndex(GameMode, Controller) != -1)
            return;

        ChangePlayersLeft(GameMode, 1);
        GameMode->AlivePlayers.Add(Controller);
    }

    void UnregisterAlive(AFortGameMode* GameMode, AController* Controller, EBotControllerKind Kind)
    {
        if (Kind == EBotControllerKind::AIController)
        {
            ChangePlayersLeft(GameMode, -1);
            return;
        }

        if (FindAlivePlayerIndex(GameMode, Controller) == -1)
            return;

        if (RemoveFromAlivePlayers_)
        {
            // Same call ClientOnPawnDied makes, with no killer / weapon / death cause: the bot left the match.
            ((void (*)(AFortGameMode*, AController*, AFortPlayerStateAthena*, AFortPlayerPawnAthena*, UFortItemDefinition*, uint8, char))RemoveFromAlivePlayers_)(GameMode, Controller, nullptr,
                                                                                                                                                     nullptr, nullptr, 0, 0);
        }

        // Fallback (or the native function kept it for some reason): AlivePlayers holds raw pointers and
        // the controller is about to be destroyed, so it must not stay in the array.
        auto Index = FindAlivePlayerIndex(GameMode, Controller);
        if (Index != -1)
        {
            GameMode->AlivePlayers.Remove(Index);
            ChangePlayersLeft(GameMode, -1);
        }
    }

    bool IsPawnDead(AFortPlayerPawnAthena* Pawn)
    {
        if (!Pawn || Pawn->bActorIsBeingDestroyed)
            return true;

        static auto IsDeadFunc = Pawn->GetFunction("IsDead");
        if (IsDeadFunc)
            return Pawn->Call<bool>(IsDeadFunc);

        return Pawn->GetHealth() <= 0.f;
    }

    bool IsEliminated(AController* Controller, EBotControllerKind Kind, AFortPlayerPawnAthena* Pawn)
    {
        if (Pawn)
            return IsPawnDead(Pawn);

        if (Kind == EBotControllerKind::PlayerController)
        {
            static auto IsInAircraftFunc = Controller->GetFunction("IsInAircraft");
            if (IsInAircraftFunc && Controller->Call<bool>(IsInAircraftFunc))
                return false;
        }

        return true;
    }

    bool SetMovementSpeed(AFortPlayerPawnAthena* Pawn, EBotMoveSpeed Speed)
    {
        static auto StyleOffset = Pawn->GetOffset("CurrentMovementStyle");
        static auto StyleEnum = SDK::FindEnum("EFortMovementStyle");
        static auto OnRepFunc = Pawn->GetFunction("OnRep_CurrentMovementStyle");

        if (StyleOffset == -1 || !StyleEnum)
        {
            static bool bLogged = false;
            if (!bLogged)
            {
                bLogged = true;
                BOT_LOG(BOT_CATEGORY_COMPAT, "Movement styles are not exposed on this build; bots keep their default speed.");
            }
            return false;
        }

        static auto WalkValue = StyleEnum->GetValue("Walking");
        static auto RunValue = StyleEnum->GetValue("Running");
        static auto SprintValue = StyleEnum->GetValue("Sprinting");

        auto Value = Speed == EBotMoveSpeed::Walk ? WalkValue : (Speed == EBotMoveSpeed::Sprint ? SprintValue : RunValue);
        if (Value == -1)
            Value = RunValue; // e.g. no dedicated sprint style on this build
        if (Value == -1)
            return false;

        auto& Style = *(uint8*)(__int64(Pawn) + StyleOffset);
        if (Style == (uint8)Value)
            return true;

        Style = (uint8)Value;
        if (OnRepFunc)
            Pawn->ProcessEvent(OnRepFunc, nullptr);

        return true;
    }
}
