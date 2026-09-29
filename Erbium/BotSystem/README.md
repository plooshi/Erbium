# Erbium Bot System

A modular system for AI-controlled players in a local/private Erbium test server. It is being built in
phases. **Implemented: Phase 1 (spawning and lifecycle), Phase 2A (movement, navigation, perception),
Phase 3 (cosmetics), and Phase 4 (building, the parts that don't depend on combat).**
Still open: Phase 2B (loot and inventory), Phase 2C (combat and healing), Phase 4 combat building
(needs 2C), and Phase 5.

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
| GUI → Misc → Bots | Spawn N / Remove all / toggles |
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
- The `GameMemberInfoArray` entry is not removed when a bot is removed. Erbium has no removal code for real players either. The effect is limited to squad UI data.
- The default appearance is fixed. Random cosmetics are Phase 3.

## Phase 2A: movement, navigation, perception

### Controller backends
`FBotSystemConfig::ControllerMode` (GUI: Misc → Bots → "Bot controller"):
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

**Selection.** For each slot the mode is `Random`, `Specific` (a random pick among the listed ids, falling back to Random if none of them are valid), or `None`. It can be set in code (`FBotCosmeticSettings`), in the GUI (Misc → Bots), or in an optional `BotCosmetics.txt` in the server's working directory:
```
Outfit=CID_028_Athena_Commando_F, CID_029_Athena_Commando_F_Halloween
Backpack=Random
Pickaxe=Random
Glider=None
```
Ids are the ones the game uses, which are also the ones listed on fortnite.gg. The GUI's reload button re-reads the file.

**Applying.** Parts are resolved the same way as Erbium's native-bot cosmetics (`Misc.cpp`): hero specializations plus backpack parts. They're shown through the pawn's `ServerChoosePart` and the player state's `CharacterParts` (both layouts), followed by the usual visual refresh. For player-controller bots the controller's `CosmeticLoadoutPC` is filled too, so `GiveStartingItems` gives them their pickaxe. If the outfit can't be applied, the bot gets the default appearance.

**Limitations.**
- AI-controller bots don't get their cosmetic pickaxe or glider yet. They have no inventory (Phase 2B) and don't skydive yet. The choice is kept in `FBotController::Cosmetics` for when they do.
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

### Build verification
There is no Windows toolchain in the development container. Each changed translation unit was checked with clang in MSVC-compatibility mode against mingw-w64 headers (`-fms-extensions -fms-compatibility -fsyntax-only`). The error output is identical to the untouched baseline: only libstdc++-internal errors that the toolchain itself causes. The authoritative build is MSBuild (`.github/workflows/Build.yml`).
