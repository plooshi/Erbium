# Erbium Bot System

A modular system for AI-controlled players in a local/private Erbium test server. It is being built in
phases; **only Phase 1 (spawning and lifecycle) is implemented so far.**

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
| **Navigation** | `UWorld::NavigationSystem` is exposed. Nothing in Erbium uses pathfinding yet. | Phase 2 will put navigation behind an interface, because nav mesh availability varies by map and version. |
| **Building** | Hooks for `ServerCreateBuildingActor` / `ServerEditBuildingActor` / `ServerRepairBuildingActor`, plus `ABuildingSMActor` | Phase 4 will go through these server functions. |
| **Cosmetics** | `CosmeticLoadoutPC` / `CustomizationLoadout` (`FFortAthenaLoadout`), `CharacterParts` (raw array or `FCustomCharacterParts`, depending on the build). `Misc.cpp` already references `FortAthenaAIBotCustomizationData`. | Phase 3 |

## Layout

```
BotSystem/
  Public/
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

### Known limitations (Phase 1)
- **Not run-time tested yet.** It has been compiled and checked (see below) but not run against a game build in this environment.
- Bots stand still (Idle). Movement, perception and combat are Phase 2.
- Bots do not jump from the bus by themselves. Erbium's aircraft code already skips `bIsABot` players for auto-jump. Phase 2 will add a bus-exit behavior.
- The `GameMemberInfoArray` entry is not removed when a bot is removed. Erbium has no removal code for real players either. The effect is limited to squad UI data.
- The default appearance is fixed. Random cosmetics are Phase 3.

### Build verification
There is no Windows toolchain in the development container. Each changed translation unit was checked with clang in MSVC-compatibility mode against mingw-w64 headers (`-fms-extensions -fms-compatibility -fsyntax-only`). The error output is identical to the untouched baseline: only libstdc++-internal errors that the toolchain itself causes. The authoritative build is MSBuild (`.github/workflows/Build.yml`).
