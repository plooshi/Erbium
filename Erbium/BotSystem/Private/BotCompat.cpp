#include "pch.h"
#include "../Public/BotCompat.h"
#include "../Public/BotEngineTypes.h"
#include "../Public/BotLog.h"
#include "../../FortniteGame/Public/FortGameMode.h"
#include "../../FortniteGame/Public/FortKismetLibrary.h"
#include "../../FortniteGame/Public/FortWeapon.h"

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
        // Plain engine AI controller: no Fortnite bot logic of its own that could need bot data we do
        // not provide, or compete with the bot system for the pawn. Virtual inventory.
        auto EngineAIController = AAIController::StaticClass();

        // Fortnite's own bot controller (the class Epic's server bots use in this version range).
        // Native inventory; its own brain is stopped at spawn (DisableBuiltInAI).
        static auto FortBotController = FindClass("FortAthenaAIBotController");

        if (FBotSystemConfig::bPreferFortniteBotController && FortBotController)
            Out.push_back({ FortBotController, EBotControllerKind::AIController });
        if (EngineAIController)
            Out.push_back({ EngineAIController, EBotControllerKind::AIController });
        if (!FBotSystemConfig::bPreferFortniteBotController && FortBotController)
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

    static AFortInventory* EnsureAIInventory(AController* Controller)
    {
        if (!Controller->GetInterface(IFortInventoryOwnerInterface::StaticClass()))
            return nullptr;

        // 0x10000 = object property cast flag: only plain object references are written directly.
        static auto WorldInventoryOffset = Controller->GetOffset("WorldInventory", 0x10000);
        static auto InventoryOffset = Controller->GetOffset("Inventory", 0x10000);
        auto Offset = WorldInventoryOffset != -1 ? WorldInventoryOffset : InventoryOffset;

        if (Offset != -1)
            if (auto Existing = *(AFortInventory**)(__int64(Controller) + Offset))
                return Existing;

        auto Inventory = UWorld::SpawnActor<AFortInventory>(AFortInventory::StaticClass(), FVector{}, FRotator{}, Controller);
        if (!Inventory)
            return nullptr;

        Inventory->InventoryType = 0;
        if (Offset != -1)
            *(AFortInventory**)(__int64(Controller) + Offset) = Inventory;
        else
            BOT_LOG(BOT_CATEGORY_COMPAT, "AI controller has no inventory property; its inventory is only known to the bot system.");

        return Inventory;
    }

    AFortInventory* EnsureWorldInventory(AController* Controller, EBotControllerKind Kind)
    {
        if (Kind != EBotControllerKind::PlayerController)
            return EnsureAIInventory(Controller);

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

    // Number of slots in the player state's raw CharacterParts array (EFortCustomPartType::NumTypes on
    // the builds that use the raw layout).
    static constexpr size_t RawCharacterPartCount = 6;

    static const UObject** FindPlayerStateCharacterParts(AFortPlayerStateAthena* PlayerState)
    {
        // Layout 1: CharacterParts is an FCustomCharacterParts struct (0x100000 = StructProperty cast flag).
        // Layout 2: CharacterParts/LocalCharacterParts is a raw fixed-size array of part pointers.
        static auto StructPartsOffset = PlayerState->GetOffset("CharacterParts", 0x100000);

        if (StructPartsOffset != -1)
        {
            static auto CustomCharacterPartsStruct = SDK::FindStruct("CustomCharacterParts");
            static auto PartsOffset = CustomCharacterPartsStruct ? CustomCharacterPartsStruct->GetOffset("Parts") : -1;

            return PartsOffset != -1 ? (const UObject**)(__int64(PlayerState) + StructPartsOffset + PartsOffset) : nullptr;
        }

        static auto RawPartsOffset = PlayerState->GetOffset("CharacterParts");
        static auto LocalPartsOffset = PlayerState->GetOffset("LocalCharacterParts");
        auto Offset = RawPartsOffset != -1 ? RawPartsOffset : LocalPartsOffset;

        return Offset != -1 ? (const UObject**)(__int64(PlayerState) + Offset) : nullptr;
    }

    bool ApplyCharacterParts(AFortPlayerStateAthena* PlayerState, AFortPlayerPawnAthena* Pawn, const FCharacterParts& Parts)
    {
        bool bApplied = false;

        static auto ServerChoosePartFunc = Pawn->GetFunction("ServerChoosePart");
        if (ServerChoosePartFunc)
        {
            for (size_t i = 0; i < Parts.size(); i++)
                if (Parts[i])
                {
                    uint8 PartType = (uint8)i;
                    auto Part = Parts[i];
                    Pawn->ServerChoosePart(PartType, Part);
                }

            bApplied = true;
        }

        if (auto CharacterParts = FindPlayerStateCharacterParts(PlayerState))
        {
            for (size_t i = 0; i < RawCharacterPartCount; i++)
                if (Parts[i])
                    CharacterParts[i] = Parts[i];

            bApplied = true;
        }

        if (!bApplied)
        {
            static bool bLogged = false;
            if (!bLogged)
            {
                bLogged = true;
                BOT_LOG(BOT_CATEGORY_COMPAT, "No way to set character parts on this build; bots use the engine default appearance.");
            }
            return false;
        }

        // Same visual refresh ServerAcknowledgePossession uses for real players.
        UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
        if (!UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization__Ptr && ApplyCharacterCustomization)
            ((void (*)(AActor*, AActor*))ApplyCharacterCustomization)(PlayerState, Pawn);

        return true;
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

        // Indices follow EFortCustomPartType: 0 = Head, 1 = Body, 3 = Backpack.
        FCharacterParts Parts{};
        Parts[0] = Head;
        Parts[1] = Body;
        Parts[3] = Backpack;

        ApplyCharacterParts(PlayerState, Pawn, Parts);
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

    AController* CreditBotKill(AFortPlayerPawnAthena* VictimPawn, AFortPlayerStateAthena* VictimState)
    {
        if (!VictimPawn || !VictimPawn->HasDamagers() || VictimPawn->Damagers.Num() == 0)
            return nullptr;

        // Most recent damager (entries are appended as damage comes in).
        auto& Damagers = VictimPawn->Damagers;
        auto Killer = Damagers[Damagers.Num() - 1].DamageCauser;
        if (!Killer || !Killer->IsA(AController::StaticClass()))
            return nullptr;

        auto KillerState = ((AController*)Killer)->PlayerState;
        if (!KillerState || KillerState == VictimState)
            return nullptr;

        if (KillerState->HasKillScore())
            KillerState->KillScore++;
        else
            KillerState->Kills++;
        KillerState->OnRep_Kills();

        if (KillerState->HasTeamKillScore())
        {
            KillerState->TeamKillScore++;
            KillerState->OnRep_TeamKillScore();
        }

        // Kill feed / elimination message for a human killer, same call ClientOnPawnDied makes.
        struct FReportKillParams
        {
            AFortPlayerStateAthena* Victim;
            uint8_t Padding[0x8];
        };
        FReportKillParams Params{ VictimState };
        KillerState->ClientReportKill(Params);

        return (AController*)Killer;
    }

    void SetPlacement(AFortPlayerStateAthena* VictimState, int TeamsLeftBefore)
    {
        if (!VictimState || !VictimState->HasPlace() || TeamsLeftBefore <= 0)
            return;

        VictimState->Place = TeamsLeftBefore;
        VictimState->OnRep_Place();
    }

    void SetDeathInfo(AFortPlayerStateAthena* VictimState, AController* Killer, AFortPlayerPawnAthena* KillerPawnOrNull, const FVector& DeathLocation)
    {
        if (!VictimState)
            return;

        auto KillerState = Killer ? Killer->PlayerState : nullptr;

        if (VictimState->HasPawnDeathLocation())
            VictimState->PawnDeathLocation = FVector(DeathLocation);

        if (!VictimState->HasDeathInfo())
            return;

        // Same fields as ClientOnPawnDied; the cause stays 0 (generic) because the damage tags of the
        // final hit are not available here.
        auto& DeathInfo = VictimState->DeathInfo;
        memset(&DeathInfo, 0, FDeathInfo::Size());
        DeathInfo.bDBNO = false;
        if (FDeathInfo::HasKiller())
            DeathInfo.Killer = KillerState;
        if (FDeathInfo::HasDeathLocation())
            DeathInfo.DeathLocation = FVector(DeathLocation);
        if (FDeathInfo::HasDeathClassSlot())
            DeathInfo.DeathClassSlot = (uint8)-1;
        if (FDeathInfo::HasFinisherOrDowner())
            DeathInfo.FinisherOrDowner = KillerState ? (AActor*)KillerState : (AActor*)VictimState;
        if (FDeathInfo::HasDistance() && KillerPawnOrNull)
        {
            auto KillerLocation = KillerPawnOrNull->K2_GetActorLocation();
            auto DX = KillerLocation.X - DeathLocation.X, DY = KillerLocation.Y - DeathLocation.Y, DZ = KillerLocation.Z - DeathLocation.Z;
            DeathInfo.Distance = (float)sqrt(DX * DX + DY * DY + DZ * DZ);
        }
        if (FDeathInfo::HasbInitialized())
            DeathInfo.bInitialized = true;

        VictimState->OnRep_DeathInfo();
    }

    void DeclareWinner(AFortGameMode* GameMode, AController* Killer)
    {
        auto GameState = GameMode->GameState;
        auto KillerState = Killer ? Killer->PlayerState : nullptr;
        if (!KillerState)
            return;

        // Only once per match (a later death with one team left must not replay the win screen).
        if (GameState->HasWinningPlayerState() && GameState->WinningPlayerState)
            return;

        if (KillerState->HasPlace())
        {
            KillerState->Place = 1;
            KillerState->OnRep_Place();
        }

        // Win screen for a human (or player-controller bot) winner, like ClientOnPawnDied.
        if (Killer->IsA<AFortPlayerControllerAthena>())
        {
            auto KillerController = (AFortPlayerControllerAthena*)Killer;
            auto KillerPawn = KillerController->Pawn;
            auto KillerWeapon = KillerPawn && KillerPawn->CurrentWeapon ? ((AFortWeapon*)KillerPawn->CurrentWeapon)->WeaponData : nullptr;
            uint8 DeathCause = 0;

            if (VersionInfo.FortniteVersion >= 16)
                KillerController->PlayWinEffects(KillerPawn, KillerWeapon, DeathCause, false);
            KillerController->ClientNotifyWon(KillerPawn, KillerWeapon, DeathCause);
            KillerController->ClientNotifyTeamWon(KillerPawn, KillerWeapon, DeathCause);
        }

        GameState->WinningTeam = KillerState->TeamIndex;
        GameState->OnRep_WinningTeam();
        if (GameState->HasWinningPlayerState())
        {
            GameState->WinningPlayerState = KillerState;
            GameState->OnRep_WinningPlayerState();
        }

        BOT_LOG(BOT_CATEGORY_SYSTEM, "Match won by team %d.", (int)KillerState->TeamIndex);
    }

    void RemoveGameMember(AFortGameMode* GameMode, AFortPlayerStateAthena* PlayerState)
    {
        auto GameState = GameMode->GameState;
        if (!PlayerState || !GameState->HasGameMemberInfoArray())
            return;

        auto& Members = GameState->GameMemberInfoArray.Members;
        for (int i = Members.Num() - 1; i >= 0; i--)
        {
            auto& Member = Members.Get(i, FGameMemberInfo::Size());
            if (Member.TeamIndex != PlayerState->TeamIndex || Member.SquadId != PlayerState->SquadId)
                continue;

            if (FUniqueNetIdRepl::HasReplicationBytes() && Member.MemberUniqueId.ReplicationBytes.Num() > 0)
                continue; // a real player's entry

            Members.Remove(i, FGameMemberInfo::Size());
            GameState->GameMemberInfoArray.MarkArrayDirty();
            return;
        }
    }

    void SetPawnInPlay(AFortPlayerPawnAthena* Pawn, bool bInPlay)
    {
        if (!Pawn)
            return;

        // Without collision the pawn would still fall under gravity: through the floor and out of the
        // world, where the game kills it (bots "died" as soon as the bus started). So movement is frozen
        // (MOVE_None) while out of play and set to falling (MOVE_Falling) when it comes back in the air.
        if (auto Movement = Pawn->CharacterMovement)
        {
            uint8 Mode = bInPlay ? 3 : 0;
            uint8 CustomMode = 0;
            Movement->SetMovementMode(Mode, CustomMode);
            if (!bInPlay)
                Movement->Velocity = FVector();
        }

        bool bHidden = !bInPlay;
        Pawn->SetActorHiddenInGame(bHidden);
        ((ACharacter*)Pawn)->SetActorEnableCollision(bInPlay);
    }

    bool IsPawnInPlay(AFortPlayerPawnAthena* Pawn)
    {
        return Pawn && !((ACharacter*)Pawn)->bHidden;
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

    FMovementInfo GetMovementInfo(AFortPlayerPawnAthena* Pawn)
    {
        FMovementInfo Info;
        auto Movement = Pawn ? (UObject*)Pawn->CharacterMovement : nullptr;
        if (!Movement)
            return Info;

        static auto ModeOffset = Movement->GetOffset("MovementMode");
        static auto MaxWalkSpeedOffset = Movement->GetOffset("MaxWalkSpeed");
        static auto MaxAccelerationOffset = Movement->GetOffset("MaxAcceleration");
        static auto GetMaxSpeedFunc = Movement->GetFunction("GetMaxSpeed");

        if (ModeOffset != -1)
            Info.Mode = *(uint8*)(__int64(Movement) + ModeOffset);
        if (MaxWalkSpeedOffset != -1)
            Info.MaxWalkSpeed = *(float*)(__int64(Movement) + MaxWalkSpeedOffset);
        if (MaxAccelerationOffset != -1)
            Info.MaxAcceleration = *(float*)(__int64(Movement) + MaxAccelerationOffset);
        if (GetMaxSpeedFunc)
            Info.MaxSpeed = Movement->Call<float>(GetMaxSpeedFunc);

        auto& Velocity = Pawn->CharacterMovement->Velocity;
        Info.Speed2D = (float)sqrt(Velocity.X * Velocity.X + Velocity.Y * Velocity.Y);

        static auto IsTickEnabledFunc = Movement->GetFunction("IsComponentTickEnabled");
        if (IsTickEnabledFunc)
            Info.TickEnabled = Movement->Call<bool>(IsTickEnabledFunc) ? 1 : 0;

        static auto IsLocallyControlledFunc = Pawn->GetFunction("IsLocallyControlled");
        if (IsLocallyControlledFunc)
            Info.LocallyControlled = Pawn->Call<bool>(IsLocallyControlledFunc) ? 1 : 0;

        return Info;
    }

    bool EnsureCanWalk(AFortPlayerPawnAthena* Pawn)
    {
        auto Movement = Pawn ? (UObject*)Pawn->CharacterMovement : nullptr;
        if (!Movement)
            return false;

        auto Info = GetMovementInfo(Pawn);
        bool bChanged = false;
        static int LoggedRepairs = 0;

        // 1. Movement component not ticking: the pawn is never simulated (hangs in the air, ignores input).
        if (Info.TickEnabled == 0)
        {
            static auto SetTickEnabledFunc = Movement->GetFunction("SetComponentTickEnabled");
            if (SetTickEnabledFunc)
            {
                bool bEnabled = true;
                Movement->Call(SetTickEnabledFunc, bEnabled);
                bChanged = true;
                if (LoggedRepairs++ < 5)
                    BOT_VERBOSE(BOT_CATEGORY_COMPAT, "Bot pawn's movement component was not ticking: enabled it.");
            }
        }

        // 2. MOVE_None: frozen. Falling lets gravity take over; the engine switches to walking on landing.
        // (While a bot rides the bus its pawn is deliberately None, but then it is not in play and this
        // is not called.)
        if (Info.Mode == 0)
        {
            uint8 Mode = 3, CustomMode = 0;
            Pawn->CharacterMovement->SetMovementMode(Mode, CustomMode);
            bChanged = true;
            if (LoggedRepairs++ < 5)
                BOT_VERBOSE(BOT_CATEGORY_COMPAT, "Bot pawn had movement mode None (frozen): set it to falling.");
        }

        // Standard player values (UE defaults for acceleration; Fortnite's running speed).
        static auto MaxWalkSpeedOffset = Movement->GetOffset("MaxWalkSpeed");
        static auto MaxAccelerationOffset = Movement->GetOffset("MaxAcceleration");

        if (MaxAccelerationOffset != -1 && Info.MaxAcceleration >= 0.f && Info.MaxAcceleration < 10.f)
        {
            *(float*)(__int64(Movement) + MaxAccelerationOffset) = 2048.f;
            bChanged = true;
        }

        if (MaxWalkSpeedOffset != -1 && Info.MaxWalkSpeed >= 0.f && Info.MaxWalkSpeed < 10.f)
        {
            *(float*)(__int64(Movement) + MaxWalkSpeedOffset) = 600.f;
            bChanged = true;
        }

        if (bChanged)
        {
            static int Logged = 0;
            if (Logged++ < 3)
                BOT_VERBOSE(BOT_CATEGORY_COMPAT, "Bot pawn had no walking speed (max speed %.0f, walk %.0f, accel %.0f): restored player defaults.", Info.MaxSpeed, Info.MaxWalkSpeed,
                        Info.MaxAcceleration);
        }

        return bChanged;
    }

    void SetDamageable(AFortPlayerPawnAthena* Pawn, bool bDamageable)
    {
        if (Pawn && Pawn->HasbCanBeDamaged())
            Pawn->bCanBeDamaged = bDamageable;
    }
}
