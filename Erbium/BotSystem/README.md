# Erbium Bot System

A modular system for AI-controlled players in a local/private Erbium test server. It is being built in
phases. **All phases are implemented; none has been run in-game yet.** Phase 1 (spawning), 2A (movement,
navigation, perception), 2B (loot, inventory), 2C (combat, healing, escaping), 3 (cosmetics),
4 (building, including combat building), 5 (decision system, difficulty, debugging), 6 (match flow:
storm, battle bus, chests, placement/win) and 7 (general fixes) are all in the code. Everything was
compile-checked (see Build verification) but not run against a game build in the development
container. The test plans below are how each phase gets verified.

## How Erbium works (and why this matters for bots)

Erbium is not an Unreal project. It is a DLL injected into a shipped Fortnite server build. So:

| Area | How Erbium does it | What that means for bots |
|---|---|---|
| **Reflection / versions** | `SDK/Defs.h`: `DEFINE_PROP` / `DEFINE_FUNC` look up properties and UFunctions **by name at runtime**. `HasX()` says whether they exist in this build. LWC (float vs double) layouts are hidden behind `FVector`/`FRotator`/`FTransform` accessors. | Most version differences are handled automatically. The rest (a different call or object layout) goes in `BotCompat`. No `#if` per version. |
| **New classes** | Not possible. The DLL can't register new `UClass`es because there is no reflection data for them. | A bot is a real `Athena_PlayerController_C` possessing a real player pawn. `FBotController` is a plain C++ object that drives it. |
| **Players** | `AFortPlayerControllerAthena` + `AFortPlayerStateAthena` + `AFortPlayerPawnAthena`. The controller's `WorldInventory` is an `AFortInventory`. | Bots use exactly these, so health, shield, inventory, movement component, collision and replication all behave as for a human player. |
| **Spawning** | `AFortGameMode::SpawnDefaultPawnFor` / `HandleStartingNewPlayer_` are hooks. Loadout and abilities are set up in `ServerAcknowledgePossession`, which is a **client → server RPC**. | Bots have no client, so the spawner runs the same steps itself. The loadout and ability code was moved out into `GiveStartingItems` / `GrantPlayerAbilities` so players and bots share one implementation. |
| **Death** | `ClientOnPawnDied` hook: drops items, updates the kill feed, calls the native `RemoveFromAlivePlayers`. | The bot manager gets notified from the same hook. It also polls, in case that hook doesn't fire for controllers with no connection. |
| **Server tick** | `UNetDriver::TickFlush` has three variants (legacy, ReplicationGraph, Iris), depending on the build. | `FBotManager::Tick` is called from all three and ignores net drivers other than the world's main one. |
| **Threading** | The ImGui GUI runs on its own thread. | The GUI never touches bot actors. It only sets atomic requests, which `Tick` carries out on the game thread. |
| **Inventory / weapons** | `AFortInventory::GiveItem`, `ServerExecuteInventoryItem`, `AFortWeapon` | Phase 2 will reuse these. It won't add a separate weapon system. |
| **Net mode** | `Misc::GetNetMode` forces **dedicated server** (1). | On a dedicated server `APlayerController::IsLocalController()` is always false, so the movement component moves a PlayerController's pawn only from *client* `ServerMove` RPCs. A client-less PlayerController bot cannot walk. **Bots that need to move use an AI controller** (see below). |
| **Navigation** | `UWorld::NavigationSystem` exists. Nothing in Erbium used pathfinding before this. | `IBotNavigation`, with runtime fallback, because nav mesh availability varies by map and version (Phase 2A). |
| **Building** | Hooks for `ServerCreateBuildingActor` / `ServerEditBuildingActor` / `ServerRepairBuildingActor`, plus `ABuildingSMActor` | Phase 4 will go through these server functions. |
| **Cosmetics** | `CosmeticLoadoutPC` / `CustomizationLoadout` (`FFortAthenaLoadout`), `CharacterParts` (raw array or `FCustomCharacterParts`, depending on the build). `Misc.cpp` already references `FortAthenaAIBotCustomizationData`. | Phase 3 |

## Layout

```
BotSystem/
  Public/
    BotEngineTypes.h AController / AAIController / ACharacter / NavigationSystemV1 views
    Navigation/      IBotNavigation strategies + FBotMovement (goals, arrival, stuck, speed)
    Perception/      shared world snapshot + per-bot FBotPerception
    Behaviors/       Idle / Searching / Moving / Following / Building / Dead
    Cosmetics/       catalogue from the build's asset registry, loadouts, application
    Building/        piece catalogue, grid + self-calibration, per-bot builder
    Inventory/       item classification + scoring, native/virtual inventory storage
    Loot/            shared pickup snapshot, claims, pickup execution
    Combat/          weapon compat (fire/reload/ammo), targeting + aiming + weapon handling
    Decision/        utility brain (priorities), difficulty presets
    World/           match phase, battle bus, storm circles, chest/ammo box snapshot
    BotConfig.h      FBotSystemConfig (server-wide, GUI-editable) + FBotConfig (per bot)
    BotState.h       EBotLifeState (exists/dead/removing) and EBotAIState (what it is doing)
    BotBehavior.h    IBotBehavior: OnEnter / Tick / OnExit, one object per AI state
    BotController.h  FBotController: per-bot driver, weak refs to controller/pawn/player state
    BotSpawner.h     creates / destroys the engine actors behind a bot
    BotManager.h     FBotManager: owns bots, ids, limits, queue, tick, cleanup, perf stats
    BotCompat.h      every version-dependent call used by the bot system
    BotLog.h         [BotSystem] [BotSpawn] [BotAI] [BotPerf] [BotCompat] log categories
  Private/           implementations
```

## Phase 1: spawning and lifecycle

### Spawn sequence (`BotSpawner::SpawnBot`)
1. Controller from `GameMode->PlayerControllerClass`, falling back to `Athena_PlayerController_C`.
2. Player state. The engine normally creates one; if it didn't, one is created and attached.
3. World inventory, created the same way as in `HandleStartingNewPlayer_`.
4. Spawn location: the caller's position or `GameMode->ChoosePlayerStart` (the same source real players use), scattered by `SpawnScatterRadius`. The first attempts use "don't spawn if colliding". The last attempt accepts an adjusted location.
5. Pawn from `GetDefaultPawnClassForController`.
6. Possess. The result is **checked** (`PC->Pawn == Pawn && Pawn->Controller == PC`). If it fails, the pawn and controller are destroyed.
7. Team + `GameMemberInfo`, abilities, starting items, default appearance, name.
8. Added to `AlivePlayers` last, so a failure never leaves a half-built bot in the match.

If any step fails, every actor created so far is destroyed.

### Removal (`BotSpawner::DestroyBot`)
Remove from `AlivePlayers` (the native `RemoveFromAlivePlayers` if Erbium found it, otherwise by hand. The array holds raw pointers, so this must happen first), then unpossess and destroy the pawn, inventory, controller and player state. Each step checks that its object still exists.

### Lifecycle
`Spawning → Active → Dead → PendingRemoval → erased`. A bot counts as dead when the `ClientOnPawnDied` hook fires, or when the maintenance check (every `MaintenanceInterval`) finds its pawn dead or gone **and** the controller is not in the battle bus. Dead bots are removed after `DeadBotCleanupDelay`. All engine references are `TWeakObjectPtr` (index and serial number are checked on every access), so an actor the game destroyed reads as `nullptr`, never as a dangling pointer.

### Using it
| Where | Action |
|---|---|
| Cheat console | `cheat spawnbot <N>`, `cheat removebot <id>`, `cheat removebots`, `cheat listbots` |
| GUI → AI tab | Spawn N / Remove all / toggles |
| Code | `FBotManager::SpawnBot()`, `SpawnBot(Loc, Rot)`, `SpawnBots(N, &Near)`, `RemoveBot(id)`, `RemoveAllBots()`, `GetActiveBots()`, `GetBotCount()`, `IsBot(controller)` |

Large requests are queued and spawned at `MaxSpawnsPerTick` per frame. `MaxBots` caps bots plus queued requests.

### Performance instrumentation
While bots exist, every `PerfReportInterval` seconds:
```
[BotPerf] bots=50 alive=50 ticks=900 avg=41.2us steady=12.0us max=9120.4us spawns=0 avgSpawn=0.00ms
```
`avg` includes spawning. `steady` excludes it. Behaviors are staggered over `ThinkInterval` (bots are split into 10 groups by id). Maintenance runs every `MaintenanceInterval`, not every frame.

### Phase 1 test plan (needs a Windows machine with a supported Fortnite build)
For each of 1, 5, 10, 25, 50 and 100 bots:
1. Start the server, wait for "Joinable", join with one client.
2. `cheat spawnbot N`. Check for N `[BotSpawn] Spawned bot` lines, that bots are visible in game and standing, and that the player count went up by N.
3. Record the `[BotPerf]` lines and the server FPS / memory in Task Manager.
4. Kill a bot. Check for `Bot #x died`, a kill feed entry, and `Removed bot #x` after about 5s.
5. `cheat removebot <id>`, then `cheat removebots`. The count should go back to baseline, with no crash.
6. Start the bus with bots alive. Bots must **not** be marked dead. Their in-bus behavior is Phase 2.
7. Spawn/remove 25 bots in a loop 10×. Memory should stay flat, with no crash.

### Known limitations (Phase 1, some lifted by 2A)
- **Not run-time tested yet.** It has been compiled and checked (see below) but not run against a game build in this environment.
- Player-controller bots stand still. AI-controller bots move (Phase 2A).
- Bots do not jump from the bus by themselves. Erbium's aircraft code already skips `bIsABot` players for auto-jump. Phase 2 will add a bus-exit behavior.
- ~~The `GameMemberInfoArray` entry is not removed when a bot is removed.~~ Fixed in Phase 7.
- The default appearance is fixed. Random cosmetics are Phase 3.

## Phase 2A: movement, navigation, perception

### Controller backends
`FBotSystemConfig::ControllerMode` (GUI: AI tab → "Bot controller"):
- **Auto** (default): try the engine `AIController`, then `FortAthenaAIBotController`, then fall back to the Phase 1 player controller.
- **AI controller**: `AAIController` subclasses. The pawn is simulated on the server, so it can move. It gets a player state (created if the class doesn't make one), a team, abilities, a name and appearance. Any built-in behavior tree is stopped (`BrainComponent->StopLogic`), so only the bot system drives the pawn.
- **Player controller**: Phase 1 behavior. The bot cannot move (see Net mode above).

AI-controller bots are **not** added to `GameMode->AlivePlayers`, because Erbium iterates it in many places and casts every entry to `AFortPlayerControllerAthena`. They count towards `GameState->PlayersLeft` only. The engine `AIController` doesn't implement Fortnite's inventory-owner interface, so these bots get no inventory until Phase 2B.

### Navigation (`Navigation/BotNavigation.h`)
| Mode | How | Needs |
|---|---|---|
| Pathfinding | `AAIController::MoveToLocation/MoveToActor`, pathfinding on | nav mesh data on the map |
| DirectPath | the same calls with `bUsePathfinding = false` | a navigation system |
| Steering | `APawn::AddMovementInput` towards the goal every frame | nothing (the pawn must be locally controlled, which AI pawns are) |

No mode teleports. All movement goes through the character movement component. A mode that fails 8 times without ever succeeding is disabled for the match, with a `[BotNavigation]` log line. A single failed request falls through to the next mode right away. Search points come from `NavigationSystemV1::K2_GetRandomReachablePointInRadius` when a nav mesh answers, or a random point on the circle otherwise.

`FBotMovement` turns goals (`MoveToLocation`, `MoveToActor`, `Follow`) into navigation requests. It handles arrival (2D radius with ±3 m height tolerance), repathing for moving targets, stuck detection (jump, then give up after `MaxStuckChecks`), and speed (sprint beyond `SprintDistance`). Speed uses the pawn's `CurrentMovementStyle`, with the values looked up by name in `EFortMovementStyle`. If the build doesn't expose that, speed stays at the default and this is logged once.

### Perception (`Perception/BotPerception.h`)
- One `GetAllActorsOfClass(FortPlayerPawnAthena)` every `PerceptionSnapshotInterval` (0.5 s), shared by all bots.
- Each bot, every `PerceptionInterval`, applies a distance cull (`DetectionRadius`) against the snapshot, sorts by distance, and runs `AController::LineOfSightTo` for the nearest `MaxLineOfSightChecks` pawns only. `LastSeenTime` is kept so later combat can remember targets briefly.
- Teams come from player states. Bots don't learn anything a player couldn't: pawns out of range are unknown, and unseen pawns are marked not visible.

### State machine
`Idle`, `Searching` (wander to random reachable points with short pauses), `Moving` (commanded destination, then Idle), `Following` (commanded target, repaths as it moves, back to Searching if it is gone), and `Dead`. Behaviors only set goals; `FBotMovement` executes them. Each think runs perception → behavior → movement.

### Test cheats
`cheat botfollow [id]`, `cheat botgoto [id]`, `cheat botwander [id]`, `cheat botidle [id]`. With no id, the command applies to all bots. `cheat listbots` now shows the state, controller kind and nav mode.

### Phase 2A test plan
1. Spawn 5 bots. The log should say `Spawned bot #n (AIController)`. If it says `PlayerController`, no AI controller class could be spawned on this build. Report the `[BotSpawn]` lines.
2. Bots should start wandering (Searching) within about 1 s. `cheat listbots` shows `nav=Pathfinding`, `DirectPath` or `Steering`, meaning the best mode that worked. Check for `[BotNavigation]` lines about disabled modes.
3. `cheat botfollow`, then run around. Bots should follow, stop about 4 m away, and sprint when far behind (unless `[BotCompat] Movement styles are not exposed` was logged).
4. `cheat botgoto` from somewhere else. Bots walk to that spot and stop.
5. Walk bots into a wall or fence. They should jump, then give up and pick a new point.
6. Kill a bot. Check for `Bot #x died`, the player count going down, and removal after 5 s.
7. Repeat steps 1–2 with 25, 50 and 100 bots and record the `[BotPerf]` lines, especially `steady=`.

### Phase 2A limitations
- AI-controller bots have no inventory or items yet (Phase 2B), so they can't fight (Phase 2C).
- Killing an AI-controller bot doesn't go through Erbium's `ClientOnPawnDied`, which is a PlayerController function. The bot is detected as dead and removed, but kill-feed and killer stats for these kills aren't handled yet. This comes with combat (2C).
- Bots don't board the battle bus. Bus and drop handling is planned with 2C and Phase 5.

## Phase 3: cosmetics

**Catalogue.** For each slot (Outfit, Backpack, Pickaxe, Glider, Contrail), `UKismetSystemLibrary::GetPrimaryAssetIdList` asks the running build's asset manager for every id of that primary asset type (`AthenaCharacter`, `AthenaBackpack`, `AthenaPickaxe`, `AthenaGlider`, `AthenaSkyDiveContrail`). This is the per-version compatibility layer: a cosmetic that doesn't exist in a build isn't in that build's list, so there's no hard-coded 12.x…29.x table to go stale. If the asset manager returns nothing for a type, the catalogue falls back to cosmetics already loaded in memory. The log shows the count and source for each slot:

```
[BotCosmetics] Outfit: 1034 available (asset registry).
```

Ids containing `random`, `npc`, `test` or `tbd` are excluded, because they aren't wearable player cosmetics.

**Loading.** Only chosen cosmetics are loaded: a soft reference comes from `GetSoftObjectReferenceFromPrimaryAssetId`, and Erbium's soft-pointer code loads it. Each one is validated before use (it must be the right item class, and an outfit must have a hero definition). An id that fails is dropped from the pool, so it's never picked again.

**Selection.** For each slot the mode is `Random`, `Specific` (a random pick among the listed ids, falling back to Random if none of them are valid), or `None`. It can be set in code (`FBotCosmeticSettings`), in the GUI (AI tab), or in an optional `BotCosmetics.txt` in the server's working directory:
```
Outfit=CID_028_Athena_Commando_F, CID_029_Athena_Commando_F_Halloween
Backpack=Random
Pickaxe=Random
Glider=None
```
Ids are the ones the game uses, which are also the ones listed on fortnite.gg. The GUI's reload button re-reads the file.

**Applying.** Parts are resolved the same way as Erbium's native-bot cosmetics (`Misc.cpp`): hero specializations plus backpack parts. They're shown through the pawn's `ServerChoosePart` and the player state's `CharacterParts` (both layouts), followed by the usual visual refresh. For player-controller bots the controller's `CosmeticLoadoutPC` is filled too, so `GiveStartingItems` gives them their pickaxe. If the outfit can't be applied, the bot gets the default appearance.

**Limitations.**
- AI-controller bots get their cosmetic pickaxe through their inventory (added with Phase 2B). Gliders aren't used yet, because bots don't skydive.
- Styles/variants use the defaults.
- Loading a cosmetic is a synchronous asset load. Spawning is spread over frames (`MaxSpawnsPerTick`), but mass spawns can still hitch. Watch `avgSpawn` in `[BotPerf]`.

**Test plan.** Spawn 25 bots: outfits and backpacks should differ, and the `[BotCosmetics]` counts should be above 0. Set `Outfit=CID_DOES_NOT_EXIST` in the file, reload and spawn: you should get a log line and a random valid outfit, with no crash. Set every slot to `None`: bots use the default appearance. `cheat botoutfit CID_... [id]` changes a living bot's outfit. `cheat listbots` shows each bot's loadout.

## Phase 4: building

**Existing code reused.** The validation and placement part of `ServerCreateBuildingActor` was moved unchanged into `AFortPlayerControllerAthena::PlaceBuildingActor`. It runs the native `CantBuild` structural/collision check, removes replaced pieces, spawns and initialises the piece, and sets level, mirroring and team. Players and bots now use the same code. The helper returns `nullptr` instead of crashing when `CantBuild_` wasn't found on a build, or when the spawn fails.

**Pieces** (`BotBuildCatalog`). Built from `GameState->AllPlayerBuildableClasses`, classified by each class default's `BuildingType` (`EFortBuildingType` Wall/Floor/Stairs/Roof, looked up by name) and `ResourceType`. No class paths are hard-coded.

**Where** (`BotBuildGrid`). Positions come from the structural grid (512 × 512 × 384 uu). For a wall this is the cell edge facing the threat; for a ramp, the next cell in the direction of travel; for a floor, the next cell; for a cone, the top of the current cell. Pivot position and wall orientation aren't assumed. The first placed piece of each type is measured with `GetActorBounds`, and the pivot offset and wall yaw are learned from it (logged as `Calibrated Wall: pivot offset (...)`). A misplaced piece is removed and placed again once with the correction.

**Pipeline** (`FBotBuilder::Build`), without ever assuming success:
1. Choose a material that has the piece and can be afforded.
2. Get the grid candidates.
3. Run the game's placement validation for each candidate.
4. Check the spawned actor exists.
5. Measure it and correct it if needed.
6. Pay.

If every candidate fails, the builder backs off for `BuildCooldown`.

**Materials.** Player-controller bots with an inventory use the game's own `CanAffordToPlaceBuildableClass` and `PayBuildableClassPlacementCost`, like players, and respect `bInfiniteMats`/`bBuildFree`. AI-controller bots have no inventory yet, so they use a per-bot budget (`FBotConfig::StartingMaterials`, 300 wood, 150 stone, 0 metal by default). This is a stand-in until Phase 2B.

**Decisions** (every `BuildDecisionInterval`, only when Idle, Searching, Moving or Following):
- **Defensive:** the bot's health or shield dropped within `DamageMemory` seconds and an enemy is visible. With probability `DefensiveBuildChance` it places a wall toward the nearest visible enemy, then holds behind it for `DefensiveHoldTime`.
- **Obstacle:** movement gave up because the bot stopped making progress. It places a ramp toward the blocked goal and walks over it.
- Every decision waits `BuildReactionTime` (±50%) before placing, and is limited by `BuildCooldown` and `MaxBuildsPerMinute`, so behaviour isn't instant or deterministic.

**Not implemented yet.**
- **Combat building** (shoot → build → reposition): needs Phase 2C.
- **Editing and repairing:** Erbium's edit and repair paths run through PlayerController RPCs with edit-tool state. They're left for a later step rather than bypassed.
- **Ramp facing:** it can't be measured from bounds. If ramps rise toward the bot, set `FBotConfig::RampYawOffset = 180` and report it, so the default can be fixed for that build.

**Test plan.**
1. Spawn a few bots, stand near them, and run `cheat botbuild wall`. Each bot should place a wall between itself and you. The first one logs `Calibrated Wall`. If a wall faces the wrong way, the log says so, and the next walls must be correct.
2. `cheat botbuild ramp` and `cheat botbuild floor`: check the pieces sit on the grid and connect to player-built pieces.
3. Shoot a bot that can see you. It should build a wall within about a second (the chance is 60% per decision).
4. `cheat botgoto` from behind a wall or cliff. Bots should get stuck, place a ramp, and walk over it.
5. Materials: AI bots stop building after 45 pieces (300 wood + 150 stone at 10 each). With `bInfiniteMats` they keep going.
6. Invalid spots (inside buildings or terrain): no crash, the build is rejected, and `builds=` in `listbots` doesn't increase.

## Phase 2B: loot and inventory

**Storage** (`Inventory/BotInventory.h`). All decisions go through one interface, which has two backends:
- **Native**: a real `AFortInventory`, changed only through Erbium's own `GiveItem`/`Remove`/`UpdateEntry`. Used by player-controller bots and by AI controllers that implement Fortnite's inventory-owner interface (`FortAthenaAIBotController`).
- **Virtual**: the bot system keeps the item list itself. Used by the plain engine `AIController`, which has no inventory-owner interface. Weapons are still equipped on the pawn with the game's `EquipWeaponDefinition`. Only counts and reserve ammo are tracked locally, and building materials go to the bot's builder.

**Changes to existing Erbium code** (needed as soon as a non-player owns real items):
- `RemoveInventoryItem` hook: it derived a PlayerController from the interface pointer with a fixed offset. It now first asks `FBotManager::HandleRemoveInventoryItem` whether the interface belongs to an AI-controller bot.
- `SetLoadedAmmo` / `SetPhantomReserveAmmo` hooks: they cast the item's owning controller to a PlayerController. For any other owner they now update the entry in the item's own `ParentInventory`.
- `FinishedTargetSpline` (pickup completion): now has an explicit PlayerController guard.

Player behaviour is unchanged in all three.

**Item evaluation** (`Inventory/BotItems.h`):
- **Categories:** Weapon, Melee, Ammo, Healing, Shield, Utility, Resource, Tool, Other. They come from the item definition class (`FortAmmoItemDefinition`, `FortResourceItemDefinition`, ...), whether a weapon uses separate ammo (`GetAmmoWorldItemDefinition_BP`), and configurable name tokens for consumables and weapon range.
- **Scores:** configurable weights in `FBotItemPriorities`.
  - Rarity adds a bonus.
  - A gun is worth much less if the bot already has an equal or better one for that range, and double if it has no gun at all.
  - Ammo only counts for carried guns.
  - Consumables are worth less once the bot already carries some.
- **Full slots:** with all five primary slots in use, an item is only picked up if it beats the worst one, which is dropped as a pickup.

**Looting.** One pickup snapshot every `LootSnapshotInterval` is shared by all bots. The brain scores the best pickup within `LootSearchRadius`, with a distance falloff. A pickup is *claimed* so other bots leave it alone. The bot walks to it, takes it the way the game completes a pickup (mark as picked up, notify, destroy the pickup), then re-equips its best weapon. A pickup it can't reach is ignored for 30 s. Chests aren't opened: Erbium opens containers through a PlayerController interaction RPC.

**Phase 3 fix included.** AI-controller bots now get their cosmetic pickaxe (or the default one) plus the playlist's starting items through their inventory, and equip it at spawn.

## Phase 2C: combat, healing, escaping

**Weapons** (`Combat/BotWeapons.h`, all detected by name at runtime):
- **Firing:** `PawnStartFire`/`PawnStopFire`, falling back to `TryActivateAbility`/`CancelAbilityHandle` on the weapon's primary ability. The chosen method is logged as `[BotCombat] Firing method on this build: ...`.
- **Reloading:** the weapon's reload ability. Virtual inventories move reserve ammo into the magazine after the weapon's `ReloadTime`.
- If no fire method exists, bots position and aim but never fake damage.

**Targeting and aim** (`Combat/BotCombat.h`):
- **Targets:** only from perception, meaning enemies in range that the bot can see or saw within `TargetMemory`. The score is distance and visibility, with a bonus for the current target, and a new target is taken only after `TargetSwitchDelay`. Enemy health isn't read, because players can't see it in BR.
- **Aim:** it doesn't start before `AimReactionTime`. It moves toward the target at up to `AimTurnSpeed` degrees per second (no snapping), and adds a random error from `AimAccuracy` that scales with distance and is re-rolled regularly. The aim point drives the AI controller's focal point, which is what the weapon fires along. It's updated every frame, so it's smooth.
- **Firing:** in bursts with pauses, only when the target is visible and the aim is within `FireToleranceDegrees`. When empty the bot reloads, and when it's out of ammo it switches weapons. The weapon is chosen for the distance (close/medium/long by name tokens, then rarity), with the pickaxe used only up close.

**Combat behaviour** (nested states):
- **Approach:** beyond the weapon's preferred range.
- **Back off:** too close for the weapon.
- **Strafe:** a sideways move every `RepositionInterval` (never standing still).
- **Search:** go to the last-seen position when the target is out of sight.
- Bots run instead of sprinting, because Fortnite pawns can't fire while sprinting.

**Healing.** Used when health is below `HealHealthThreshold` or shield below `HealShieldThreshold`, with no visible enemy within `SafeHealDistance` and no recent damage. The bot equips the item and holds "fire", which is how consumables are used, until the game consumes it. Taking damage interrupts it.

Virtual-inventory bots can't use consumables through the game's item abilities, which need a real item. With `bEmulateConsumablesForVirtualInventory` they apply the standard effect from `FBotConsumableEffects` after its use time. **This is an emulation**, so turn it off if it isn't wanted.

**Escaping.** When low (`RetreatHealthFraction`) and threatened, or when facing an enemy with nothing to fight with, the bot sprints `EscapeDistance` away from the threat.

**AI-controller bot deaths.** The bot drops its items as pickups, and the most recent damager gets the kill (kill score and kill feed, the same calls `ClientOnPawnDied` makes). Placement and victory handling for AI victims isn't done.

## Phase 4 addition: combat building
Build decisions are now also evaluated during Looting and **Combat**. A bot that is hit while fighting a visible enemy places a wall toward it (`DefensiveBuildChance`), holds behind it, and returns to Combat, where it repositions, aims and shoots again. Reaction time, cooldown and the per-minute limit keep this non-deterministic.

## Phase 5: decision system, difficulty, debugging

**Brain** (`Decision/BotBrain.h`). Every `DecisionInterval` the bot scores five priorities, each with its own small function and a configurable weight (`FBotConfig::PriorityWeights`):

| Priority | State | Scored from |
|---|---|---|
| Survival | Escaping | low health and threatened, or an enemy and nothing to fight with |
| Combat | Combat | nearest known enemy (visible, or seen recently) and a usable weapon, × `Aggression` |
| Healing | Healing | missing health/shield, carrying the right item, safe |
| Loot | Looting | best pickup score |
| Search | Searching | constant baseline |

A switch needs a 15% margin and at least 0.75 s in the current state (except Survival), so bots don't flip-flop. The brain only switches between autonomous states. Commanded states (cheat Idle/Moving/Following) and Building aren't overridden.

**Difficulty** (`BotDifficulty::Apply`): Easy, Normal or Hard. Presets set reaction time, accuracy, tracking speed, target-switch delay, detection and engagement range, aggression, reposition rate, build reaction/chance/rate, and sprinting. They never give information a player wouldn't have. New bots use `FBotSystemConfig::DefaultDifficulty` (GUI). `cheat botdifficulty <easy|normal|hard> [id]` changes existing bots.

**Debugging.**
- `cheat botdebug <id>` shows:
  - id, state and difficulty
  - the decision with all five scores
  - health and shield
  - position and destination with nav mode
  - target distance and visibility, and combat activity
  - weapon and its ammo
  - inventory, builds and materials, and cosmetics
- `cheat botwatch <id>` logs the same every 3 s (`botwatch 0` stops).
- `[BotAI]`, `[BotCombat]`, `[BotLoot]` and the other categories log lifecycle events only. Per-decision detail is behind the GUI's "Verbose bot logging".

**Performance.**
- The pawn and pickup snapshots are shared by all bots and rebuilt every 0.5 s / 1 s.
- Line-of-sight traces are capped per bot.
- The loot search is cached for 1 s, and decisions run every 0.4 s.
- Thinks are staggered, and only aim smoothing and steering run every frame.

Measure with `[BotPerf]` as in Phase 1.

## Test plan for 2B / 2C / 5
1. Spawn 10 bots on a map with floor loot. `cheat botdebug 1`: the inventory shows the pickaxe and starting items, and the state moves Searching → Looting → Searching. Inventory mode is `Virtual` with the default controller.
2. Drop a weapon near a bot. It should walk over, pick it up, and equip it (`botdebug` weapon line).
3. Walk into view of an armed bot. It should switch to Combat, turn toward you over roughly half a second, and fire in bursts. The log states the firing method. If bots aim but never shoot, enable GUI → "Prefer Fortnite bot controller", spawn new bots, and report both results.
4. Hide behind a wall. The bot should go to where it last saw you.
5. Damage a bot while it fights. It should build a wall (some of the time) and then keep fighting. Bring it low: it should escape, and later heal if it carries heals.
6. Kill a bot. Its items should drop and your kill count go up. With AI controllers there's no placement or victory handling yet.
7. `cheat botdifficulty easy`, then `hard`: aim delay, accuracy and building rate should be clearly different.
8. 50 and 100 bots with loot and fighting: record `[BotPerf]`.

### Known limitations after this round
- **Not run-time tested.** The things most likely to need adjustment in-game:
  - whether weapons fire through the engine `AIController` (there's a fallback option)
  - ramp facing
  - speed via movement styles
- No chest opening, no bus/skydive handling, no editing or repairing.
- AI-controller victims get kill credit and drops, but no placement or win handling.

## Phase 6: match flow (storm, battle bus, chests, placement)

**Two crash fixes in existing Erbium code** (both reachable once bots fight):
- **`ClientOnPawnDied`, when a bot kills a human:** the killer's controller was always treated as a PlayerController. It got quest stats, the win screen and the victory crown, all PlayerController-only calls on what may be an AI controller. Those calls now run only when the killer really is a PlayerController. Kill score, the kill feed and the winning team still apply to bot killers.
- **Storm damage on 25.20+:** it's a gameplay effect Erbium applied per player inside `ServerAcknowledgePossession`, which bots never send. It's been moved unchanged into `AFortPlayerControllerAthena::ApplyStormDamageEffect`, which players and bots both call.

**World info** (`World/BotWorld.h`, all of it information players have as well):
- **Match phase:** from `GameState` before 25.20 and `GamePhaseLogic` from 25.20.
- **Battle bus and drop window:** `DropStartTime`/`DropEndTime`, or `FlightInfo`.
- **Storm circles:** the current circle, interpolated while shrinking, and the next one.
- **Chests and ammo boxes:** a snapshot every `ContainerSnapshotInterval`.
- **Storm flags on 25.20+:** Erbium updates `bIsInAnyStorm`/`bIsInsideSafeZone` only for `AlivePlayers`, so `UpdatePawnStormState` does the same for bot pawns. That's what makes storm damage apply to them.

**Storm.** A Storm priority in the brain is scored like this:
- 1.8 when the bot is outside the current circle (taking damage), which beats combat.
- 1.1 while the storm is shrinking and the bot is outside the next circle.
- 0.9 in the last 30 s before a shrink.
- Otherwise 0.45.

The `Rotating` behaviour heads for the nearest side of the next circle, about 55% of the radius in. Wander points and chests prefer the inside of the next circle.

**Battle bus** (`bBotsUseBattleBus`, GUI toggle).
- **Boarding:** when the phase becomes Aircraft, every AI-controlled bot boards. Its pawn is hidden, has no collision and isn't perceived, the same as a player in the bus who has no pawn in the world. Each bot gets a random jump time in the drop window.
- **Jumping:** at that time the pawn is placed at the bus with a small spread, like Erbium's late-game jump, and `BeginSkydiving(true)` starts the fall.
- **Skydiving:** steering input takes the bot toward a chest near its jump point (inside the next circle), and it lands and starts playing.
- **Late spawns:** bots spawned after the bus started drop in over the safe zone.
- **Player-controller bots** are in `AlivePlayers`, so the game itself puts them in the bus. Since Phase 7 they also get a jump time and jump (see Phase 7).

**Chests and ammo boxes.** They compete with ground loot in the brain's Loot score (`ChestValue`, `AmmoBoxValue`, with distance falloff). The bot claims the container, walks to it, stands for `ContainerSearchTime` (like holding interact), then opens it with `UFortLootPackage::SpawnLootHook`. That's the same function Erbium's `ServerAttemptInteract` uses for players. The loot drops as pickups, which the bot collects next.

**Placement and win for AI-controlled bot victims.**
1. The victim's `Place` is set to the number of players left.
2. It stops counting as alive.
3. The killer is credited.
4. If one player is left, the killer is declared the winner: win screen for a human, then `WinningTeam`/`WinningPlayerState`.

Since Phase 7 this counts teams, which is correct for squads too, and is skipped during warmup.

**Test plan.**
1. Spawn 20 bots during warmup, then start the bus. The log should show `Battle bus: 20 bot(s) boarded.` Bots vanish from the island, then appear in the sky during the drop window, glide down near chests and start looting. `cheat botdebug <id>` shows `Match: Aircraft`, and later `SafeZones`.
2. Chests near landed bots should open (bounce and loot drop) after about 1 s of the bot standing at them.
3. When the storm closes, bots outside the next circle switch to `Rotating` and move in. On 25.20+ builds, check that bots standing in the storm take damage.
4. Let a bot kill you. There should be no crash, and the kill should show in the feed. Then kill the others down to the last one: the winner is declared, with the win screen if that's you.
5. Spawn bots mid-match with GUI → Spawn: they should drop in over the safe zone.

**Limitations.**
- Gliding/auto-deploy is left to the game's movement. If a build needs an explicit deploy, bots will free-fall until they land. Report it if that happens.
- The glider cosmetic isn't applied for AI-controlled bots.
- ~~Squad placement and team elimination aren't handled.~~ Fixed in Phase 7.

## Phase 7: general fixes

Found by reviewing all previous phases against each other and against the Erbium code they touch.

| # | Problem | Fix |
|---|---|---|
| 1 | Placement and win counted *players*, so they were wrong in squad modes | `FBotManager::GetAliveTeams()` counts teams: `AlivePlayers` plus AI-controlled bots that are still alive. A bot gets a placement when its whole team is out, and the killer's team wins when it's the only team left. |
| 2 | A bot dying in **warmup** could set placements or even declare a winner | Placement and the win check run only after the match has started (`GUI::gsStatus == StartedMatch`). |
| 3 | **Player-controller bots stayed in the bus forever.** Erbium's auto-jump skips `bIsABot` players | They get a jump time in the drop window and jump through `ServerAttemptAircraftJump` (on the aircraft component when present), the same function a client calls. Erbium's hook then respawns them in the air. |
| 4 | Eliminations of AI-controlled bots had no `DeathInfo` (elimination feed, spectating) | `BotCompat::SetDeathInfo` fills the same fields `ClientOnPawnDied` does for players: killer, location, distance, finisher. |
| 5 | The squad-UI `GameMemberInfo` entry was never removed | `BotCompat::RemoveGameMember` on removal. Only entries without a unique id (bots) are touched, never a human's. |
| 6 | Erbium's `RemoveInventoryItem` hook runs on **every shot by anyone**, and asked every bot's controller for its interface each time | The interface pointer is cached at spawn, so the hook now only compares pointers. |
| 7 | Loot claims of a bot removed while alive were never released, and a bot's "unreachable loot" list only grew | Claims are released on removal, and expired entries are pruned every 10 s. |
| 8 | The world state was refreshed after queued spawns, and not at all while no bots existed. The first bot spawned during the storm phase thought the phase was "Unknown" and stayed on the warmup island | The world refreshes first each tick. |

Still open, because they need in-game results first:
- whether weapons fire through the engine `AIController`
- ramp facing (`RampYawOffset`)
- movement-style speeds
- glider auto-deploy
- the glider cosmetic for AI-controlled bots


## Round 8: auto spawn, AI tab, client console

- **Bots now spawn on their own.** With `FBotSystemConfig::bAutoSpawn` (on by default) the manager waits for the first client connection once the server is joinable, then after `AutoSpawnDelay` (5 s) fills the match to `AutoSpawnCount` (20) bots. Before this, bots only appeared through the GUI button or `cheat` commands. The log shows `[BotSpawn] Player connected: auto spawning ...` and `Auto spawn: requesting N bot(s)`; a rejected spawn logs `Spawn rejected (enabled=.., bots=.., server ready=..)`.
- **AI tab in the GUI.** All bot controls moved from Misc into their own tab:
  - "Enable AI bots": off removes every bot and blocks spawning; on lets auto spawn run again via "Fill to auto spawn count".
  - "Pause bot AI": bots stay but stop thinking, moving and shooting.
  - Auto spawn toggle and count, manual spawn/fill/remove, and the behavior, cosmetic and debug settings.
- **F8 console (ErbiumClient).** The client DLL created the console once at injection and crashed or did nothing if the viewport did not exist yet, and Fortnite's console key list usually only has the tilde key. `ErbiumClient/Private/Client.cpp` now retries creating the console every client tick until it exists and adds F8 (and Tilde) to `UInputSettings::ConsoleKeys`. This only helps if you inject ErbiumClient into your game client; the server DLL cannot give a client a console.


## Round 9: spawn fallback, status panel, console retry

- **Spawn falls back per step.** Before, only spawning the controller itself fell back to the next controller class. A failure later (player state, pawn, player start, possession) gave up. Now `BotSpawner::SpawnBot` runs steps 1-6 with each candidate in turn (engine AIController, FortAthenaAIBotController, then the player controller). A failed attempt's actors are destroyed before the next one.
- **Auto spawn retries.** If a fill produced no bot at all, it is retried every 10 s, up to 3 times. A manual "Remove All Bots" is never refilled automatically.
- **Status panel (AI tab)** shows:
  - the server state
  - whether the bot system is receiving server ticks
  - connected players
  - the auto-spawn stage
  - the last spawn error
  - the last rejected spawn

  Use it to report why no bots appear.
- **Console keys retried (ErbiumClient).** The previous fix looked up the input settings once, through a getter that caches its first result. When the DLL was injected before the game created that object, F8 was never added. `EnsureConsoleKeys` now looks it up until found, and re-checks the keys about every 3 s.


## Round 10: server crash after loading in

Two Erbium hooks assumed the pawn or instigator always has a player controller. Both run for bots once they are in the world:

| Hook | When it ran for a bot | Fix |
|---|---|---|
| `AFortPlayerPawnAthena::EndSkydiving` (FortPlayerPawnAthena.cpp) | every time a pawn lands: bus drop, drop-in spawn, or a fall | Returns early unless the controller is an `AFortPlayerControllerAthena`; quest events need one. |
| `ABuildingSMActor::OnDamageServer` (BuildingSMActor.cpp) | a bot hits a tree, rock or wall with its pickaxe | Material harvesting (writes into `WorldInventory`) only for player controllers that have one; everything else goes to the original function. |

## Where to find everything

**Branch:** `claude/erbium-ai-bot-system-sus0gw` on `wieseichwiesenicht-art/Erbium-AI-Fork2`, one commit per phase on top of `main`.
- Browse: https://github.com/wieseichwiesenicht-art/Erbium-AI-Fork2/tree/claude/erbium-ai-bot-system-sus0gw
- All changes against `main`: https://github.com/wieseichwiesenicht-art/Erbium-AI-Fork2/compare/main...claude/erbium-ai-bot-system-sus0gw
- Locally: `git fetch origin claude/erbium-ai-bot-system-sus0gw && git checkout claude/erbium-ai-bot-system-sus0gw`, then `git log --oneline main..HEAD` for the phase commits.

**New code**, all under `Erbium/BotSystem/`:

| Folder / file | What |
|---|---|
| `Public/BotManager.h`, `Private/BotManager.cpp` | owns all bots: spawn/remove, tick, match flow, commands, debug text, perf log |
| `BotController`, `BotBehavior`, `BotState`, `BotConfig` | per-bot driver, behavior interface, states/enums, all settings |
| `BotSpawner`, `BotCompat`, `BotEngineTypes`, `BotLog` | spawn/teardown, version/game compatibility layer, engine class views, logging |
| `Navigation/` | `BotNavigation` (pathfinding / direct / steering), `BotMovement` (goals, stuck handling, speed) |
| `Perception/` | shared pawn snapshot, per-bot visibility |
| `Behaviors/` | all behaviors: Idle, Searching, Moving, Following, Building, Looting, Combat, Healing, Escaping, Rotating, InBus, Skydiving, Dead |
| `Inventory/` | item classification/scoring, native/virtual inventory |
| `Loot/` | pickup snapshot, claims, chests |
| `Combat/` | weapon compat (fire/reload/ammo), targeting, aiming |
| `Building/` | piece catalogue, grid + calibration, builder |
| `Cosmetics/` | cosmetic catalogue, loadouts |
| `Decision/` | brain (priorities), difficulty |
| `World/` | match phase, bus, storm, containers |

**Changed Erbium files** (every change is commented in the code):

| File | Change |
|---|---|
| `FortniteGame/Private/FortPlayerControllerAthena.cpp` / `.h` | shared helpers extracted unchanged (`GrantPlayerAbilities`, `GiveStartingItems`, `PlaceBuildingActor`, `ApplyStormDamageEffect`); bot cheats replace the old `spawnbot`; death notification to the bot manager; `ClientOnPawnDied` only calls player-controller functions on a player-controller killer |
| `FortniteGame/Private/FortInventory.cpp` | `RemoveInventoryItem` / `SetLoadedAmmo` / `SetPhantomReserveAmmo` are safe for non-player item owners |
| `FortniteGame/Private/FortPlayerPawnAthena.cpp` | player-controller guard in pickup completion |
| `Engine/Private/NetDriver.cpp` | bot manager tick in all three `TickFlush` variants |
| `Erbium/Private/GUI.cpp` | Misc tab → Bots section |
| `Erbium.vcxproj`, `Erbium.vcxproj.filters` | new files registered |

**Cheats** (`cheat <command>` in game):
- `spawnbot <n>`, `removebot <id>`, `removebots`, `listbots`
- `botfollow`, `botgoto`, `botwander`, `botidle` (optional `[id]`)
- `botbuild <wall|floor|ramp|cone> [id]`, `botoutfit <CID> [id]`
- `botdifficulty <easy|normal|hard> [id]`
- `botdebug <id>`, `botwatch <id|0>`

### Build verification
There is no Windows toolchain in the development container. Each changed translation unit was checked with clang in MSVC-compatibility mode against mingw-w64 headers (`-fms-extensions -fms-compatibility -fsyntax-only`). The error output is identical to the untouched baseline: only libstdc++-internal errors that the toolchain itself causes. The authoritative build is MSBuild (`.github/workflows/Build.yml`).
