#include "pch.h"
#include "../Public/BotSpawner.h"
#include "../Public/BotCompat.h"
#include "../Public/BotController.h"
#include "../Public/BotEngineTypes.h"
#include "../Public/BotLog.h"
#include "../Public/Cosmetics/BotCosmetics.h"
#include "../Public/Loot/BotLoot.h"
#include "../../FortniteGame/Public/FortGameMode.h"

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
    if (auto PlayerStart = GameMode->ChoosePlayerStart(Controller))
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

// Spawns the first controller candidate that the engine accepts.
static AController* SpawnController(AFortGameMode* GameMode, FTransform& Transform, EBotControllerKind& OutKind)
{
    for (auto& Candidate : BotCompat::GetBotControllerCandidates(GameMode))
    {
        if (auto Controller = (AController*)UWorld::SpawnActor(Candidate.Class, Transform))
        {
            OutKind = Candidate.Kind;
            return Controller;
        }

        BOT_LOG(BOT_CATEGORY_SPAWN, "Could not spawn controller %s, trying the next candidate.", Candidate.Class->GetName().ToString().c_str());
    }

    return nullptr;
}

namespace BotSpawner
{
    bool SpawnBot(FBotController& Bot, const FBotSpawnParams& Params)
    {
        auto World = UWorld::GetWorld();
        auto GameMode = World ? (AFortGameMode*)World->AuthorityGameMode : nullptr;
        if (!GameMode || !GameMode->GameState)
        {
            BOT_LOG(BOT_CATEGORY_SPAWN, "Bot #%u: no game mode, cannot spawn.", Bot.GetId());
            return false;
        }

        auto& Config = Bot.GetConfig();

        // 1. Controller
        FRotator ControllerRotation = Params.Rotation;
        FTransform ControllerTransform(Params.Location, ControllerRotation);
        EBotControllerKind Kind = EBotControllerKind::PlayerController;

        auto Controller = SpawnController(GameMode, ControllerTransform, Kind);
        if (!Controller)
        {
            BOT_LOG(BOT_CATEGORY_SPAWN, "Bot #%u: failed to spawn a controller.", Bot.GetId());
            return false;
        }

        // 2. Player state  3. Inventory
        auto PlayerState = BotCompat::EnsurePlayerState(Controller, ControllerTransform);
        Bot.BindController(Controller, Kind, PlayerState);

        if (!PlayerState)
        {
            BOT_LOG(BOT_CATEGORY_SPAWN, "Bot #%u: failed to create player state.", Bot.GetId());
            DestroyBot(Bot);
            return false;
        }

        // Native inventory where the controller supports one, otherwise virtual (see BotInventory.h).
        auto Inventory = BotCompat::EnsureWorldInventory(Controller, Kind);
        Bot.GetInventory().Initialize(Inventory);

        // 4. Location  5. Pawn
        auto PawnClass = BotCompat::GetBotPawnClass(GameMode, Controller);
        AFortPlayerPawnAthena* Pawn = nullptr;

        for (int Attempt = 0; Attempt < MaxPawnSpawnAttempts && PawnClass && !Pawn; Attempt++)
        {
            FTransform SpawnTransform;
            if (!SelectSpawnTransform(GameMode, Controller, Params, Attempt, SpawnTransform))
                break;

            // Try to avoid spawning inside geometry/other bots first; on the last attempt accept an adjusted location.
            auto CollisionHandling = Attempt + 1 < MaxPawnSpawnAttempts ? AdjustIfPossibleButDontSpawnIfColliding : AdjustIfPossibleButAlwaysSpawn;
            Pawn = (AFortPlayerPawnAthena*)UWorld::SpawnActor(PawnClass, SpawnTransform, Controller, CollisionHandling);
        }

        if (!Pawn)
        {
            BOT_LOG(BOT_CATEGORY_SPAWN, "Bot #%u: failed to spawn pawn (no valid location or pawn class).", Bot.GetId());
            DestroyBot(Bot);
            return false;
        }

        // 6. Possess
        if (!BotCompat::PossessPawn(Controller, Kind, Pawn, PlayerState) || !Bot.OnPossessed(Pawn))
        {
            BOT_LOG(BOT_CATEGORY_SPAWN, "Bot #%u: possession failed.", Bot.GetId());
            DestroyActorIfAlive(Pawn); // not yet owned by the bot, so DestroyBot would not see it
            DestroyBot(Bot);
            return false;
        }

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

        // Cosmetics before starting items: for player-controller bots the chosen pickaxe goes into
        // CosmeticLoadoutPC, which GiveStartingItems reads.
        Bot.Cosmetics = BotCosmetics::ChooseLoadout();
        bool bDressed = BotCosmetics::ApplyLoadout(Bot, Bot.Cosmetics);

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
