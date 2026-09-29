# Erbium Bot System

A modular system for AI-controlled players in a local/private Erbium test server. It is being built in
phases. **Implemented: Phase 1 (spawning and lifecycle), Phase 2A (movement, navigation, perception).**
Next: Phase 2B (loot and inventory), then Phase 2C (combat and healing).

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
    Behaviors/       Idle / Searching / Moving / Following / Dead
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

### Build verification
There is no Windows toolchain in the development container. Each changed translation unit was checked with clang in MSVC-compatibility mode against mingw-w64 headers (`-fms-extensions -fms-compatibility -fsyntax-only`). The error output is identical to the untouched baseline: only libstdc++-internal errors that the toolchain itself causes. The authoritative build is MSBuild (`.github/workflows/Build.yml`).
