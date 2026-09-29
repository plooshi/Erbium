#include "pch.h"
#include "../Public/BotSpawner.h"
#include "../Public/BotCompat.h"
#include "../Public/BotController.h"
#include "../Public/BotEngineTypes.h"
#include "../Public/BotLog.h"
#include "../Public/Cosmetics/BotCosmetics.h"
#include "../Public/Loot/BotLoot.h"
#include "../../FortniteGame/Public/FortGameMode.h"
#include <atomic>

// ESpawnActorCollisionHandlingMethod
static constexpr uint8_t AdjustIfPossibleButAlwaysSpawn = 2;
static constexpr uint8_t AdjustIfPossibleButDontSpawnIfColliding = 3;

static constexpr int MaxPawnSpawnAttempts = 4;

static FVector Scatter(const FVector& Origin, float Radius)
{
    if (Radius <= 0.f)
        return Origin;

    auto Angle = (double)UKismetMathLibrary::RandomFloatInRange(0.f, 6.2831853f);
    auto Distance = (double)UKismetMathLibrary::RandomFloatInRange(0.f, Radius);

    return FVector(Origin.X + cos(Angle) * Distance, Origin.Y + sin(Angle) * Distance, Origin.Z);
}

// Picks where the pawn should appear. Attempt > 0 means the previous location was blocked, so a
// different player start / a wider scatter is used.
static bool SelectSpawnTransform(AFortGameMode* GameMode, AController* Controller, const FBotSpawnParams& Params, int Attempt, FTransform& OutTransform)
{
    auto Radius = Params.ScatterRadius * (1.f + Attempt);

    // FRotator's conversion to FQuat is non-const in the SDK, hence the local copies.
    if (Params.bUseLocation)
    {
        FRotator Rotation = Params.Rotation;
        OutTransform = FTransform(Scatter(Params.Location, Radius), Rotation);
        return true;
    }

    // Same source of spawn points real players use (see AFortGameMode::SpawnDefaultPawnFor).
    // Fortnite's ChoosePlayerStart can refuse controllers that are not player controllers, which made
    // every AI-controller spawn fail and fall back to (immobile) player-controller bots. Then: ask with
    // a real player's controller, and finally pick a player start actor ourselves.
    AActor* PlayerStart = GameMode->ChoosePlayerStart(Controller);

    if (!PlayerStart && !Controller->IsA<AFortPlayerControllerAthena>())
    {
        TArray<AFortPlayerControllerAthena*> Players;
        Utils::GetAll<AFortPlayerControllerAthena>(Players);
        for (int i = 0; i < Players.Num() && !PlayerStart; i++)
            if (Players[i] && !Players[i]->bActorIsBeingDestroyed)
                PlayerStart = GameMode->ChoosePlayerStart(Players[i]);
        Players.Free();
    }

    if (!PlayerStart)
    {
        static auto WarmupStartClass = FindClass("FortPlayerStartWarmup");
        static auto PlayerStartClass = FindClass("PlayerStart");

        for (auto Class : { WarmupStartClass, PlayerStartClass })
        {
            if (!Class)
                continue;

            TArray<AActor*> Starts;
            Utils::GetAll(Class, Starts);
            if (Starts.Num() > 0)
                PlayerStart = Starts[rand() % Starts.Num()];
            Starts.Free();

            if (PlayerStart)
                break;
        }
    }

    if (PlayerStart)
    {
        auto StartTransform = PlayerStart->GetTransform();
        FRotator Rotation{};
        Rotation.Yaw = UKismetMathLibrary::RandomFloatInRange(0.f, 360.f);

        OutTransform = FTransform(Scatter(StartTransform.Translation, Radius), Rotation);
        return true;
    }

    return false;
}

static void DestroyActorIfAlive(AActor* Actor)
{
    if (Actor && !Actor->bActorIsBeingDestroyed)
        Actor->K2_DestroyActor();
}

// Starting loadout for bots that do not go through AFortPlayerControllerAthena::GiveStartingItems
// (AI-controller bots): the cosmetic pickaxe (Phase 3) or the default one, plus the playlist's
// starting items, with the same filter GiveStartingItems applies.
static void GiveAIStartingItems(FBotController& Bot, AFortGameMode* GameMode)
{
    auto& Inventory = Bot.GetInventory();

    const UFortItemDefinition* Pickaxe = nullptr;
    if (auto CosmeticPickaxe = (UAthenaPickaxeItemDefinition*)Bot.Cosmetics.Get(EBotCosmeticSlot::Pickaxe))
        Pickaxe = CosmeticPickaxe->WeaponDefinition;

    static auto DefaultPickaxe = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
    if (!Pickaxe)
        Pickaxe = DefaultPickaxe;

    if (Pickaxe)
        Inventory.Give(Pickaxe, 1, 0);

    static auto SmartItemDefClass = FindClass("FortSmartBuildingItemDefinition");
    for (int i = 0; i < GameMode->StartingItems.Num(); i++)
    {
        auto& StartingItem = GameMode->StartingItems.Get(i, FItemAndCount::Size());
        auto Item = StartingItem.Item;

        // The playlist's pickaxe is replaced by the one chosen above.
        if (!Item || !StartingItem.Count || (SmartItemDefClass && Item->IsA(SmartItemDefClass)) || Item->IsA<UFortWeaponMeleeItemDefinition>())
            continue;

        Inventory.Give(Item, StartingItem.Count, 0);
    }
}

static std::wstring MakeBotName(const FBotController& Bot)
{
    return std::wstring(Bot.GetConfig().NamePrefix) + L" (#" + std::to_wstring(Bot.GetId()) + L")";
}

static std::atomic<const char*> LastError = "none";
static std::atomic<const char*> LastAIError = "none";

static bool Fail(FBotController& Bot, const char* ClassName, const char* Reason)
{
    LastError = Reason;
    BOT_VERBOSE(BOT_CATEGORY_SPAWN, "Bot #%u with %s: %s", Bot.GetId(), ClassName, Reason);
    return false;
}

// Destroys the actors of a failed attempt and forgets them, so the next candidate starts clean.
static void DiscardAttempt(FBotController& Bot, AController* Controller, AFortPlayerStateAthena* PlayerState, AFortInventory* Inventory, AFortPlayerPawnAthena* Pawn)
{
    if (Controller && Pawn && Controller->Pawn == Pawn)
        Controller->UnPossess();

    DestroyActorIfAlive(Pawn);
    DestroyActorIfAlive(Inventory);
    DestroyActorIfAlive(Controller);
    DestroyActorIfAlive(PlayerState);

    Bot.BindController(nullptr, EBotControllerKind::PlayerController, nullptr);
    Bot.InventoryOwnerInterface = nullptr;
}

// Steps 1-6 with one controller class. On failure everything created is discarded again.
static bool TrySpawnWith(FBotController& Bot, const FBotSpawnParams& Params, AFortGameMode* GameMode, const BotCompat::FControllerClassChoice& Candidate)
{
    auto ClassNameString = Candidate.Class->GetName().ToString();
    auto ClassName = ClassNameString.c_str();
    auto Kind = Candidate.Kind;

    // 1. Controller
    FRotator ControllerRotation = Params.Rotation;
    FTransform ControllerTransform(Params.Location, ControllerRotation);

    auto Controller = (AController*)UWorld::SpawnActor(Candidate.Class, ControllerTransform);
    if (!Controller)
        return Fail(Bot, ClassName, "could not spawn the controller");

    // 2. Player state
    auto PlayerState = BotCompat::EnsurePlayerState(Controller, ControllerTransform);
    if (!PlayerState)
    {
        DiscardAttempt(Bot, Controller, nullptr, nullptr, nullptr);
        return Fail(Bot, ClassName, "could not create a player state");
    }
    Bot.BindController(Controller, Kind, PlayerState);

    // 3. Inventory: native where the controller supports one, otherwise virtual (see BotInventory.h).
    auto Inventory = BotCompat::EnsureWorldInventory(Controller, Kind);
    Bot.GetInventory().Initialize(Inventory);
    if (Inventory && Kind == EBotControllerKind::AIController)
        Bot.InventoryOwnerInterface = Controller->GetInterface(IFortInventoryOwnerInterface::StaticClass());

    // 4. Location  5. Pawn
    auto PawnClass = BotCompat::GetBotPawnClass(GameMode, Controller);
    if (!PawnClass)
    {
        DiscardAttempt(Bot, Controller, PlayerState, Inventory, nullptr);
        return Fail(Bot, ClassName, "the game mode has no pawn class for this controller");
    }

    AFortPlayerPawnAthena* Pawn = nullptr;
    bool bHasLocation = false;

    for (int Attempt = 0; Attempt < MaxPawnSpawnAttempts && !Pawn; Attempt++)
    {
        FTransform SpawnTransform;
        if (!SelectSpawnTransform(GameMode, Controller, Params, Attempt, SpawnTransform))
            break;
        bHasLocation = true;

        // Try to avoid spawning inside geometry/other bots first; on the last attempt accept an adjusted location.
        auto CollisionHandling = Attempt + 1 < MaxPawnSpawnAttempts ? AdjustIfPossibleButDontSpawnIfColliding : AdjustIfPossibleButAlwaysSpawn;
        Pawn = (AFortPlayerPawnAthena*)UWorld::SpawnActor(PawnClass, SpawnTransform, Controller, CollisionHandling);
    }

    if (!Pawn)
    {
        DiscardAttempt(Bot, Controller, PlayerState, Inventory, nullptr);
        return Fail(Bot, ClassName, bHasLocation ? "the pawn could not be spawned" : "no player start was found");
    }

    // 6. Possess
    if (!BotCompat::PossessPawn(Controller, Kind, Pawn, PlayerState) || !Bot.OnPossessed(Pawn))
    {
        DiscardAttempt(Bot, Controller, PlayerState, Inventory, Pawn);
        return Fail(Bot, ClassName, "the controller did not possess the pawn");
    }

    return true;
}

namespace BotSpawner
{
    const char* GetLastError()
    {
        return LastError;
    }

    const char* GetLastAIControllerError()
    {
        return LastAIError;
    }

    bool SpawnBot(FBotController& Bot, const FBotSpawnParams& Params)
    {
        auto World = UWorld::GetWorld();
        auto GameMode = World ? (AFortGameMode*)World->AuthorityGameMode : nullptr;
        if (!GameMode || !GameMode->GameState)
        {
            LastError = "no game mode (map not loaded yet)";
            BOT_LOG(BOT_CATEGORY_SPAWN, "Bot #%u: no game mode, cannot spawn.", Bot.GetId());
            return false;
        }

        auto& Config = Bot.GetConfig();

        auto Candidates = BotCompat::GetBotControllerCandidates(GameMode);
        if (Candidates.empty())
        {
            LastError = "no usable controller class on this build";
            BOT_LOG(BOT_CATEGORY_SPAWN, "Bot #%u: no controller class to spawn.", Bot.GetId());
            return false;
        }

        bool bSpawned = false;
        for (size_t i = 0; i < Candidates.size() && !bSpawned; i++)
        {
            bSpawned = TrySpawnWith(Bot, Params, GameMode, Candidates[i]);
            if (bSpawned)
                break;

            if (Candidates[i].Kind == EBotControllerKind::AIController)
                LastAIError = GetLastError();
            if (i + 1 < Candidates.size())
                BOT_VERBOSE(BOT_CATEGORY_SPAWN, "Bot #%u: trying the next controller class%s.", Bot.GetId(),
                        Candidates[i + 1].Kind == EBotControllerKind::PlayerController ? " (player controller: this bot will not be able to move)" : "");
        }

        if (!bSpawned)
        {
            BOT_LOG(BOT_CATEGORY_SPAWN, "Bot #%u: every controller class failed (last error: %s).", Bot.GetId(), GetLastError());
            Bot.MarkPendingRemoval();
            return false;
        }

        auto Controller = Bot.GetController();
        auto Kind = Bot.GetControllerKind();
        auto PlayerState = Bot.GetPlayerState();
        auto Pawn = Bot.GetPawn();
        auto Inventory = Bot.GetInventory().GetNativeInventory();
        LastError = "none";

        BotCompat::DisableBuiltInAI(Controller, Kind);

        // 7. Player setup
        // Single-argument UFunction calls pass the argument's address, so it must be a mutable local.
        float MaxHealth = Config.MaxHealth;
        float StartingShield = Config.StartingShield;
        Pawn->SetMaxHealth(MaxHealth);
        if (StartingShield > 0.f)
            Pawn->SetShield(StartingShield);

        BotCompat::AssignTeam(GameMode, Controller, PlayerState);
        AFortPlayerControllerAthena::GrantPlayerAbilities(PlayerState);

        // 25.20+: storm damage is an effect Erbium applies per player in ServerAcknowledgePossession,
        // which bots never send.
        if (VersionInfo.FortniteVersion >= 25.20)
            AFortPlayerControllerAthena::ApplyStormDamageEffect(PlayerState);

        // Cosmetics before starting items: for player-controller bots the chosen pickaxe goes into
        // CosmeticLoadoutPC, which GiveStartingItems reads.
        Bot.Cosmetics = BotCosmetics::ChooseLoadout();
        bool bDressed = BotCosmetics::ApplyLoadout(Bot, Bot.Cosmetics);
        Bot.bDressed = bDressed;

        if (Config.bGiveStartingItems)
        {
            if (Kind == EBotControllerKind::PlayerController && Inventory)
                AFortPlayerControllerAthena::GiveStartingItems((AFortPlayerControllerAthena*)Controller);
            else
                GiveAIStartingItems(Bot, GameMode);
        }

        BotLoot::EquipBestLoadout(Bot);

        if (!bDressed)
            BotCompat::ApplyDefaultAppearance(PlayerState, Pawn);
        BotCompat::SetPlayerName(GameMode, Controller, Kind, PlayerState, MakeBotName(Bot).c_str());

        // 8. Alive players (last, so a failure above never leaves a half-built bot in the match)
        BotCompat::RegisterAlive(GameMode, Controller, Kind);
        Bot.bRegisteredAlive = true;

        return true;
    }

    bool RespawnPawn(FBotController& Bot, const FVector& Location)
    {
        auto World = UWorld::GetWorld();
        auto GameMode = World ? (AFortGameMode*)World->AuthorityGameMode : nullptr;
        auto Controller = Bot.GetController();
        auto PlayerState = Bot.GetPlayerState();
        if (!GameMode || !Controller || !PlayerState)
            return false;

        auto PawnClass = BotCompat::GetBotPawnClass(GameMode, Controller);
        if (!PawnClass)
            return false;

        FRotator Rotation{};
        FTransform Transform(Location, Rotation);
        auto Pawn = (AFortPlayerPawnAthena*)UWorld::SpawnActor(PawnClass, Transform, Controller, AdjustIfPossibleButAlwaysSpawn);
        if (!Pawn)
            return false;

        if (!BotCompat::PossessPawn(Controller, Bot.GetControllerKind(), Pawn, PlayerState) || !Bot.OnPossessed(Pawn))
        {
            DestroyActorIfAlive(Pawn);
            return false;
        }

        float MaxHealth = Bot.GetConfig().MaxHealth;
        Pawn->SetMaxHealth(MaxHealth);

        if (!BotCosmetics::ApplyLoadout(Bot, Bot.Cosmetics))
            BotCompat::ApplyDefaultAppearance(PlayerState, Pawn);
        BotLoot::EquipBestLoadout(Bot);

        BOT_VERBOSE(BOT_CATEGORY_SPAWN, "Bot #%u got a new pawn (its previous one was removed).", Bot.GetId());
        return true;
    }

    void DestroyBot(FBotController& Bot)
    {
        auto World = UWorld::GetWorld();
        auto GameMode = World ? (AFortGameMode*)World->AuthorityGameMode : nullptr;

        auto Controller = Bot.GetController();
        auto Pawn = Bot.GetPawn();
        // Player controllers' WorldInventory, or the inventory created for an AI controller.
        auto Inventory = Bot.GetInventory().GetNativeInventory();

        // AlivePlayers stores raw pointers: unregister before the controller can be destroyed.
        if (GameMode && Controller && Bot.bRegisteredAlive)
        {
            BotCompat::UnregisterAlive(GameMode, Controller, Bot.GetControllerKind());
            Bot.bRegisteredAlive = false;
        }

        if (GameMode)
            BotCompat::RemoveGameMember(GameMode, Bot.GetPlayerState());

        if (Controller && Pawn && Controller->Pawn == Pawn)
            Controller->UnPossess();

        DestroyActorIfAlive(Pawn);
        DestroyActorIfAlive(Inventory);
        DestroyActorIfAlive(Controller);

        // AController::Destroyed normally cleans up its player state; this covers builds where it does
        // not. Bot.GetPlayerState() returns nullptr if that already happened.
        DestroyActorIfAlive(Bot.GetPlayerState());

        Bot.MarkPendingRemoval();
    }
}
