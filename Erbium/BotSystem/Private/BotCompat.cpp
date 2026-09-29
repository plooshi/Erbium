#include "pch.h"
#include "../Public/BotCompat.h"
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
        for (; Struct; Struct = Struct->GetSuper())
            if (Struct == Parent)
                return true;

        return false;
    }

    const UClass* GetBotControllerClass(AFortGameMode* GameMode)
    {
        if (GameMode->HasPlayerControllerClass())
        {
            auto Class = GameMode->PlayerControllerClass;

            if (Class && IsChildOf(Class, AFortPlayerControllerAthena::StaticClass()))
                return Class;
        }

        static auto Fallback = FindObject<UClass>(L"/Game/Athena/Athena_PlayerController.Athena_PlayerController_C");
        if (!Fallback)
            BOT_LOG(BOT_CATEGORY_COMPAT, "No usable player controller class found on this build.");

        return Fallback;
    }

    const UClass* GetBotPawnClass(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller)
    {
        if (auto Class = GameMode->GetDefaultPawnClassForController(Controller))
            return Class;

        return GameMode->DefaultPawnClass;
    }

    AFortPlayerStateAthena* EnsurePlayerState(AFortPlayerControllerAthena* Controller, const FTransform& Transform)
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

    AFortInventory* EnsureWorldInventory(AFortPlayerControllerAthena* Controller)
    {
        if (auto Existing = Controller->WorldInventory)
            return Existing;

        if (!Controller->HasWorldInventoryClass() || !Controller->WorldInventoryClass)
            return nullptr;

        auto Inventory = UWorld::SpawnActor<AFortInventory>(Controller->WorldInventoryClass, FVector{}, FRotator{}, Controller);
        if (!Inventory)
            return nullptr;

        Inventory->InventoryType = 0;
        Controller->WorldInventory = Inventory;

        return Inventory;
    }

    bool PossessPawn(AFortPlayerControllerAthena* Controller, AFortPlayerPawnAthena* Pawn, AFortPlayerStateAthena* PlayerState)
    {
        Controller->Possess(Pawn);

        if (Controller->Pawn != Pawn || Pawn->Controller != Controller)
            return false;

        // The original spawnbot notes this crashes on 27+ when left unset.
        if (Controller->HasMyFortPawn())
            Controller->MyFortPawn = Pawn;

        if (PlayerState && Pawn->PlayerState != PlayerState)
        {
            Pawn->PlayerState = PlayerState;
            Pawn->OnRep_PlayerState();
        }

        return true;
    }

    void AssignTeam(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller, AFortPlayerStateAthena* PlayerState)
    {
        auto GameState = GameMode->GameState;

        PlayerState->TeamIndex = AFortGameMode::PickTeam(GameMode, 0, Controller);

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

    void SetPlayerName(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller, AFortPlayerStateAthena* PlayerState, const wchar_t* Name)
    {
        FString BotName = FString(Name);

        if (std::floor(VersionInfo.FortniteVersion) < 9)
            Controller->ServerChangeName(BotName);
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

    static int32 FindAlivePlayerIndex(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller)
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

    bool IsInAlivePlayers(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller)
    {
        return FindAlivePlayerIndex(GameMode, Controller) != -1;
    }

    void AddToAlivePlayers(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller)
    {
        if (!GameMode->HasAlivePlayers() || IsInAlivePlayers(GameMode, Controller))
            return;

        auto GameState = GameMode->GameState;
        GameState->PlayersLeft++;
        GameState->OnRep_PlayersLeft();

        GameMode->AlivePlayers.Add(Controller);
    }

    void RemoveFromAlivePlayers(AFortGameMode* GameMode, AFortPlayerControllerAthena* Controller)
    {
        if (!IsInAlivePlayers(GameMode, Controller))
            return;

        if (RemoveFromAlivePlayers_)
        {
            // Same call ClientOnPawnDied makes, with no killer / weapon / death cause: the bot left the match.
            ((void (*)(AFortGameMode*, AFortPlayerControllerAthena*, AFortPlayerStateAthena*, AFortPlayerPawnAthena*, UFortItemDefinition*, uint8, char))RemoveFromAlivePlayers_)(
                GameMode, Controller, nullptr, nullptr, nullptr, 0, 0);
        }

        // Fallback (or the native function kept it for some reason): AlivePlayers holds raw pointers and
        // the controller is about to be destroyed, so it must not stay in the array.
        auto Index = FindAlivePlayerIndex(GameMode, Controller);
        if (Index != -1)
        {
            GameMode->AlivePlayers.Remove(Index);

            auto GameState = GameMode->GameState;
            if (GameState->PlayersLeft > 0)
            {
                GameState->PlayersLeft--;
                GameState->OnRep_PlayersLeft();
            }
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

    bool IsEliminated(AFortPlayerControllerAthena* Controller, AFortPlayerPawnAthena* Pawn)
    {
        if (Pawn)
            return IsPawnDead(Pawn);

        static auto IsInAircraftFunc = Controller->GetFunction("IsInAircraft");
        if (IsInAircraftFunc && Controller->Call<bool>(IsInAircraftFunc))
            return false;

        return true;
    }
}
