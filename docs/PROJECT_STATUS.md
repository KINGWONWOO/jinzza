# JINZZA — Development Status Report

**Generated**: 2026-09-01, by Claude (Claude Code)
**Covers**: all Claude-assisted work to date (sessions of 2026-08-26, 2026-08-30, and four rounds on 2026-09-01)
**Companion files**: `todo.txt` (repo root) is the chronological, dated session-by-session log this report is organized from — read it for exact dates/order of events. `docs/game.docx` / `docs/game_extracted.txt` is the authoritative game design doc. `docs/steam_session_and_settings_reference.md` is the Steam/settings adaptation-plan cross-check against the user's earlier multiplayer project.

This document reorganizes that history **by system**, not by date, so it reads as "what exists and how it works" rather than "what happened when." Treat it as a snapshot — it will drift out of date as work continues; `todo.txt` remains the live log.

---

## 1. What JINZZA is

JINZZA (진짜를 찾아라 — "Find the Real One") is a solo-developed Unreal Engine 5.8 C++ social-deduction party game. Each round:

- One player is secretly the **Real One**.
- Several other players are **Imitators**, disguised to look/sound identical to the Real One (same face material variant, same voice filter).
- One player is the **Judge**, who must figure out who the Real One is by observing free-time behavior, a written Q&A round, and 1:1 interviews, then makes a final decision.
- The game's core design principle (stated directly in the design doc) is **정보 비대칭이 곧 서스펜스다** — "information asymmetry is the suspense." The Judge must never be able to infer anything from replicated network data; role knowledge is deliberately leaked to each client only via targeted RPCs, never broadcast state.

Multiplayer is Steam-session-based and **invite-only by design** — there is no public matchmaking or room browser. This is a deliberate simplification for a friends-only party game, not a missing feature.

**Repo**: `github.com/KINGWONWOO/jinzza` (origin configured locally). **Solo developer** (confirmed 2026-09-01) — several scope calls below exist specifically because of that.

---

## 2. Current repo state (as of this report)

The working tree has **331 changed paths**, none committed since the initial scaffold commit. This includes:
- The Content-reorganization move (everything under `Content/` migrated into `Content/JINZZA/...` via the live editor's asset tools — a proper engine-side move, so references and World Partition actors stayed intact; not a filesystem move).
- All C++ source changes described in this report.
- Deletions of template placeholder content (prototyping meshes/materials, Variant_Horror/Shooter sample blueprints) that came bundled with the UE5 First Person template and aren't part of this game.

Nothing in this report has been committed to git. Commits happen only when explicitly requested.

---

## 3. Systems implemented

### 3.1 Steam session & networking (`jinzzaGameInstance.h/.cpp`)

`UjinzzaGameInstance` owns the entire Steam online-session lifecycle:

- **`HostSession(Settings)`** — destroys any existing session, then creates a new one with the given `FJinzzaMatchSettings`, and on success `ServerTravel`s to `Lvl_Lobby`.
- **Joining is invite-only.** There is no room-list search/browse flow (`UjinzzaJoinWidget`, `RefreshSessionList`, `FindSessions`, `JoinSessionAtIndex` were all removed early on) — the only join path is accepting a Steam overlay invite, handled via `OnSessionUserInviteAccepted`.
- **`UpdateLiveSessionSettings(Settings)`** — pushes `NumPublicConnections` and the searchable match-settings keys (room name, max players, judge count, vote count, phase speed, role-assign method) to the *already-live* Steam session via `IOnlineSession::UpdateSession`, without destroying/recreating it. Needed because the in-lobby Room Settings kiosk (§3.3) used to only update the local replicated `AjinzzaLobbyGameState::MatchSettings` — a widened player cap never reached Steam or friends looking at the invite until this was added.
- **`SetRichPresenceStatus(StatusText)`** — publishes Steam friends-list rich presence (e.g. "In Lobby"), called after a successful `CreateSession`/`JoinSession`. Confirmed by reading the engine source directly that `OnlineSubsystemSteam` implements `IOnlinePresence` (`FOnlinePresenceSteam::SetPresence` → `ISteamFriends::SetRichPresence`) — no extra plugin needed. Currently only ever publishes `"In Lobby"`; wiring in per-round-phase text is a one-line call away once something (the round-phase system, §3.5) needs to.
- **`EndGameReturnToLobby()`** — host-only, travels everyone back to `Lvl_Lobby`, session stays alive.
- **`DestroySession()`**, plus the full set of `OnCreate/OnJoin/OnDestroy/OnUpdateSessionComplete` delegate handlers.

**NetDriver / P2P relay fix**: session creation/joining was correct from the start, but nothing told Unreal to route actual game traffic through Steam's P2P relay — `ServerTravel`/`ClientTravel` were using the default raw-IP `NetDriver`, which only works when players are directly reachable (same LAN or manually port-forwarded). Found by cross-checking against the user's own earlier multiplayer project (`github.com/KINGWONWOO/Unreal_Game_Noob`). Fixed by matching that project's config exactly:
- `Config/DefaultEngine.ini`: `[/Script/Engine.GameEngine] NetDriver` override to `/Script/SteamSockets.SteamSocketsNetDriver` (falls back to `IpNetDriver` if unavailable; scoped to `GameEngine` only, so PIE multiplayer testing is untouched), plus `OnlineSubsystemSteam` P2P-relay settings (`bAllowP2PPacketRelay`, connection/cleanup timeouts, `bInitServerOnClient`, VAC, `GameServerQueryPort`) and `bHasVoiceEnabled=true` under `[OnlineSubsystem]` (ahead of the voice system, §5).
- `jinzza.uproject`: enabled the **SteamSockets** plugin (separate from OnlineSubsystemSteam, not on by default — the NetDriver override does nothing without it). This is why an **editor restart** was needed on 2026-08-30 — Live Coding can't hot-load a newly-enabled plugin.
- Added `steam_appid.txt` (480, matching `SteamDevAppId`) at the project root so the Steamworks SDK initializes when launched from editor/PIE.

The App ID (480) is Valve's public test ID, a placeholder — see §8, "things only the user can do."

### 3.2 Main menu & shared UI style

- **Host Game** creates the Steam session immediately with default `FJinzzaMatchSettings` and travels straight to `Lvl_Lobby` — no pre-create "Host Setup" screen (that flow was removed; its fields moved into the in-lobby kiosk, §3.3).
- **`JinzzaUI` namespace** (`UI/jinzzaUIStyle.h/.cpp`): shared rounded-brush button/panel/heading styling — a dark noir palette with gold/crimson accents matching the judge/vote theme — applied across the main menu, lobby, room settings, settings, and game-end widgets, plus a short fade-in on the main menu's button page. **No texture/font assets used** — everything is hand-built in C++ (Slate brushes/colors), per this project's no-UMG-designer constraint (documented in `UjinzzaMainMenuWidget`'s header comment).

### 3.3 Lobby (`jinzzaLobbyGameMode/State/PlayerController`, `AjinzzaRoomSettingsKiosk`, `UI/jinzzaLobbyWidget`)

- `AjinzzaLobbyGameMode` sets `PlayerStateClass = AjinzzaPartyPlayerState` (not the base `APlayerState`) specifically so seamless travel into `Lvl_Game` never has to worry about the carried-over PlayerState being the wrong type — `AjinzzaGameGameMode::AssignRoles()` casts every PlayerState to this type.
- **Room Settings kiosk** (`AjinzzaRoomSettingsKiosk`), placed physically in `Lvl_Lobby`: walk up, press **E**, and a panel opens showing the settings that used to live in a pre-lobby "Host Setup" screen (room name, max players, judge/vote count, phase speed, role-assign method). Host-only editable, using the same `HasAuthority()` + local-controller idiom the lobby's Start Match button already used. Writes straight into `AjinzzaLobbyGameState::MatchSettings` (replicates live to everyone already in the lobby) and, since round 2 of the 2026-09-01 sessions, also calls `UjinzzaGameInstance::UpdateLiveSessionSettings()` so the advertised Steam session reflects the change too (§3.1).
  - Proximity to trigger the "Press E" prompt is **polled on a 0.2s timer** from `AjinzzaLobbyPlayerController`, not driven by overlap events — the lobby's `ADefaultPawn` has no collision set up for overlaps.
  - "Interact" is a classic Action Mapping (E) added to `DefaultInput.ini` *alongside* Enhanced Input, which coexist fine.
- **Start Match** (`UjinzzaLobbyWidget::OnStartMatchClicked`): host-only, calls `World->ServerTravel("/Game/JINZZA/Level/Lvl_Game")`. `bUseSeamlessTravel = true` on both `AjinzzaLobbyGameMode` and `AjinzzaGameGameMode`, so all connected players travel together, carrying their PlayerState across.

### 3.4 Settings system (`jinzzaGameUserSettings.h/.cpp`, `UI/jinzzaSettingsWidget`)

A full 4-tab Settings screen (Graphics / Audio / Controls / Gameplay), backed by `UjinzzaGameUserSettings` (registered via `GameUserSettingsClassName` in `DefaultEngine.ini`), replacing an old placeholder. Deliberately mirrors the architecture Epic's own **Lyra** sample project uses (`UGameUserSettings` + Enhanced Input mapping overrides) — built on a real reference rather than invented, per explicit request.

- **Graphics**: window mode / resolution / vsync / frame cap, plus all 9 individual scalability categories — all real `UGameUserSettings` base-class calls.
- **Audio**: Master / Music / SFX / Voice sliders. No `SoundClass`/`SoundMix` assets exist in Content yet, so these are backed by **runtime-transient** `USoundClass`/`USoundMix` objects (a real, shipped-indie-game pattern) — Master is fully audible today; Music/SFX/Voice will govern future audio content once it's assigned to those child classes.
  - **Microphone section**: `EJinzzaMicInputMode` (Push-to-Talk / Open Mic) + `MicDeviceId`, config-persisted like the rest of the class. A device dropdown is populated from `UAudioCaptureBlueprintLibrary::GetAvailableAudioInputDevices` (required adding `AudioCapture` to `jinzza.Build.cs`'s `PrivateDependencyModuleNames`). **Nothing consumes these settings yet** — `UVoiceDisguiseComponent`/`UProximityVoiceComponent` (§5, Week 6) don't exist. This just ensures the preference exists and is stored before that component needs to read it.
- **Controls**: mouse sensitivity + invert-Y (wired into `AjinzzaCharacter::DoAim`), and key rebinding for the button-press Input Actions (Jump/Shoot/SwapWeapon/Sprint) via per-player overrides applied onto a `DuplicateObject`'d **runtime copy** of each Input Mapping Context (`AjinzzaPlayerController::BuildRuntimeMappingContext`) — the shared `.uasset` IMCs are never mutated.
- **Gameplay**: subtitles toggle (flag only — no subtitle content yet), colorblind mode + strength via the engine's real accessibility API (`UWidgetBlueprintLibrary::SetColorVisionDeficiencyType`).

### 3.5 Round-phase state machine (Week 4 — `jinzzaRoundTypes.h`, `jinzzaGameGameState`, `jinzzaRoundPhaseSubsystem`)

The first slice of actual match gameplay (everything in §3.1–3.4 is menu/lobby/settings plumbing).

- **`jinzzaRoundTypes.h`**: shared enums — `EJinzzaRoundPhase` (`RoleAssignment → SelfIntroduction → FreeTime1 → QuestionTime → MidEvaluation → FreeTime2 → Interview → FinalDecision → RoundComplete`, 8 phases + terminal state, matching design doc §6), `EJinzzaPartyRole` (`RealOne`/`Imitator`/`Judge`), `EJinzzaFaceType` (A/B/C), `EJinzzaVoiceFilter` (High/Low/Robot).
- **`AjinzzaGameGameState`** (`AGameStateBase`, set as `Lvl_Game`'s `GameStateClass`): replicates `CurrentPhase` (`RepNotify` → `OnPhaseChanged` delegate, mirroring the design-doc-referenced `AFruitGameState` pattern) and `PhaseEndServerTime` — a single timestamp against `GetServerWorldTimeSeconds()`, **not** a ticking countdown (the standard low-bandwidth technique), plus `MidEvaluationsRemaining`.
- **`UjinzzaRoundPhaseSubsystem`** (`UGameInstanceSubsystem`, matches the design doc's class list §11 exactly): server-only phase driver. `StartRound()` → `EnterPhase(RoleAssignment)` (synchronous — broadcasts `OnServerPhaseEntered` immediately, this matters for §6 below), then `AdvancePhase()` walks the remaining 7 phases on an `FTimerHandle`, writing each transition into `AjinzzaGameGameState`.
  - **Judgment calls** (both commented in code, both explicitly undecided in the design doc):
    - *Mid-evaluation looping for `VoteCount` > 1* (doc §17 lists this as undecided): implemented as repeating `MidEvaluation → FreeTime2` as a pair, `VoteCount` times, before `Interview`. `VoteCount = 1` collapses to exactly the doc's worked 6-player timing example.
    - *`PhaseSpeed` multipliers* (Slow/Normal/Fast) aren't numbered in the doc: used **1.3× / 1× / 0.75×**, applied only to `FreeTime1`/`FreeTime2`/`Interview` per the doc's own description of what that preset scales ("자유시간·면담 시간 일괄 비율 조정"). Needs tuning from real playtesting per doc §17.
  - Question-time cycle count is **hardcoded** to the doc's default of 2 (130s total) — not yet wired to a lobby setting; `FJinzzaMatchSettings` has no field for it yet.

### 3.6 Roles & disguise (Week 5 — `jinzzaPartyPlayerState`, `jinzzaDisguiseComponent`, `jinzzaGameGameMode::AssignRoles`, `jinzzaGamePlayerController`)

- **`AjinzzaPartyPlayerState`** (`APlayerState`; set as the `PlayerStateClass` for *both* `Lvl_Lobby` and `Lvl_Game`): holds `bIsGhost` / `FaceType` / `VoiceFilter` as **openly-replicated** properties (they're visually/aurally observable in-game by everyone anyway, so hiding them at the network layer buys nothing) but **deliberately does not replicate `ServerRole`** — it's not even a `UPROPERTY`, it's a plain server-only C++ member. This is the load-bearing implementation of the design doc's information-asymmetry principle (§1).
- **`AjinzzaDisguiseComponent`** (`UActorComponent`, added to `AjinzzaCharacter` so every character subclass gets one): applies `FaceType` to the third-person mesh (the one other players see — `GetMesh()` is already `SetOwnerNoSee`'d) via a dynamic material instance driving a `"FaceIndex"` scalar param. **No-op today** — no `MI_Face_A/B/C` content exists yet (doc §13-2) — but harmless, and will activate automatically once that material parameter exists (same pattern as the audio settings in §3.4).
- **`AjinzzaGameGameMode::AssignRoles()`**: Fisher-Yates shuffle of `GameState->PlayerArray` → 1 Real One + 1 Judge + the rest Imitators; Imitators clone the Real One's `FaceType`/`VoiceFilter`. Also fixed `DefaultPawnClass`, which had been left as the bare `ADefaultPawn` from initial project generation (nobody had a mesh, so disguise had nothing to apply to) — now loads `BP_FirstPersonCharacter`. **This is a real behavior change**: anyone testing `Lvl_Game` now spawns as the actual character instead of a flying camera. (Verified live via PIE — see §7.)
- **`AjinzzaGamePlayerController::Client_ReceiveRoleAssignment`** (Client Reliable RPC) + `GetLocalPartyRole()`/`GetKnownRealOne()`: this is how role knowledge actually reaches each player. `AssignRoles()` sends the Real One "you're the Real One", each Imitator "you're an Imitator, here's who the Real One is" (an `APlayerState*` reference), and the Judge "you're the Judge" with nothing else — targeted client RPCs for private per-player notification, rather than trying to make replication itself conditionally hide data.
- **`AjinzzaLobbyGameMode`** also sets `PlayerStateClass = AjinzzaPartyPlayerState` (§3.3) specifically so this all survives seamless travel cleanly.

**Deliberately not built** (flagged as scope-trim simplifications, not oversights — see §5 for why this project trims rather than speculatively builds):
- **2-judge mode**: `AssignRoles()` always assigns exactly 1 judge regardless of `FJinzzaMatchSettings::JudgeCount`. The design doc itself marks 2-judge as experimental/stretch with vote/interview rules undecided (doc §9, §15).
- **`RoleAssignMethod == "Host Picks"`**: falls back to Random. No picker UI exists for the host to choose the Real One before the round starts.
- **Lobby-customization snapshot cloning**: the design doc's fuller "역할 배정 시 위장 복제" (clone appearance on role assignment) — `UCharacterCustomizationComponent` (class list §11) doesn't exist, so there's no lobby appearance system yet to capture a look from. `AssignRoles()` only clones `FaceType`/`VoiceFilter`.
- **Voice filter isn't consumed anywhere yet** — stored on `AjinzzaPartyPlayerState`, ready to be read once `UVoiceDisguiseComponent`/`UProximityVoiceComponent` (Week 6) exist.

---

## 4. Bug found and fixed: role assignment raced against player connections

This is the most significant correctness issue found to date, and the reason multi-client testing infrastructure (§7) was built out.

### Symptom
In a fresh multiplayer match, only whichever single player's connection completed first (almost always the host) would ever receive a role. Every other player would silently get no role assignment at all — a total break of the core gameplay loop, but one that a single-player PIE session could never reveal, since a solo test always hits the (intentional, correctly-working) "need at least 3 players" bail-out path instead.

### Root cause
`AjinzzaGameGameMode::StartPlay()` called `UjinzzaRoundPhaseSubsystem::StartRound()` synchronously. `StartRound()` immediately calls `EnterPhase(RoleAssignment)`, which **synchronously** broadcasts `OnServerPhaseEntered` — which `AjinzzaGameGameMode` had bound straight to `AssignRoles()`. So role assignment ran at the exact moment the GameMode's `StartPlay()` fired — i.e., essentially the instant the server-side world finished initializing, **before** any remote client (each going through its own independent `Login`/`PostLogin` handshake, especially over real seamless travel from `Lvl_Lobby`) had actually finished connecting and gotten its `PlayerState` added to `GameState->PlayerArray`.

### Evidence
Confirmed, not just suspected — via a genuine multi-client PIE test (see §7 for how this was set up, since it required working around a real tooling gap):
1. Configured a 3-client listen-server PIE session and loaded directly into `Lvl_Game`.
2. After the warmup period, queried the live server-side `GameState` and confirmed **all 3** `AjinzzaPartyPlayerState` actors existed in `PlayerArray`.
3. The log nonetheless showed: `AssignRoles: need at least 3 players, got 1 - skipping role assignment` — logged **before** the other two clients had finished connecting, and never re-evaluated afterward (nothing re-triggers `AssignRoles()`).

This reproduced identically on two separate pre-fix test runs.

### Fix
`jinzzaGameGameMode.h`/`.cpp`:
- `StartPlay()` no longer calls `StartRound()` — it only binds the `OnServerPhaseEntered` delegate.
- New `PostLogin(APlayerController* NewPlayer)` override: on every login, (re)starts a **1.5-second debounce timer** (guarded by a new `bRoundStarted` flag) that calls a new `TryStartRound()` function.
- `TryStartRound()` sets `bRoundStarted = true` and calls `RoundPhase->StartRound()`.

Net effect: the round starts once **no new player has joined for 1.5 seconds**, rather than on a synchronous trigger from world startup, or on a fixed expected head-count. A head-count-based alternative (threading an "expected player count" from the lobby through `UjinzzaGameInstance`) was considered and rejected: `FJinzzaMatchSettings::MaxPlayers` is the room's configured *cap*, not who actually shows up, and an exact-count wait could hang forever if a player disconnects mid-travel and never reconnects. The debounce approach needs no cross-level plumbing and degrades gracefully.

### Verification of the fix
1. User compiled via **Hot Reload** (not Live Coding — confirmed in the log: `UnrealBuildTool` ran, recompiled `jinzzaGameGameMode.cpp`, "Result: Succeeded", `UnrealEditor-jinzza-3762.dll` reloaded).
2. Re-ran the **identical** 3-client PIE test. Result: all 3 PlayerStates present as before, but **the premature warning did not fire at all**, across the full 6-second warmup — where it had fired reliably, every time, pre-fix.
3. No new errors/asserts/fatals from the run.

**Caveat**: this is strong negative evidence (the bug's symptom is now absent) plus a straightforward code read, but not a hard *positive* assertion that `AssignRoles()` completed successfully with 3 players — the fields that would prove that (`ServerRole`, `FaceType`, `VoiceFilter`, `CurrentPhase`, `LocalRole`) are all private and unreadable through this session's object-introspection tooling, which was confirmed to only read *public* reflected properties (cross-checked: the inherited, public `PlayerArray` read fine; every private field tried — even `UPROPERTY`-marked ones — errored).

To close that last gap, a **temporary debug log** was added to `AssignRoles()`'s success path:
```cpp
// TEMP DEBUG (remove once the PostLogin/TryStartRound fix is trusted - see todo.txt round 4):
UE_LOG(Logjinzza, Warning, TEXT("[TEMP DEBUG] AssignRoles succeeded with %d players - RealOne=%s Judge=%s Imitators=%d"),
	Players.Num(), *RealOne->GetPlayerName(), *Judge->GetPlayerName(), Players.Num() - 2);
```
**This line should be removed once a positive-confirmation test run has been observed and trusted** — it's marked `TEMP DEBUG` in the source specifically so it's easy to find and delete later.

---

## 5. Why things get trimmed instead of speculatively built

This project is solo-developed (confirmed 2026-09-01). Where the design doc itself flags a mechanic as experimental/stretch or explicitly undecided (2-judge mode's vote/interview rules, "Host Picks" role-assignment UI, "candidate vote" mode for choosing the Real One), the consistent choice across every session has been to implement the documented **fallback-to-default** behavior (e.g. always 1 judge; "Host Picks" silently behaves as Random) rather than inventing rules for something the design itself hasn't settled. This was an explicit, confirmed preference in conversation with the user, not an assumption — see §3.6 for the concrete instances.

---

## 6. Testing & tooling infrastructure

Prior sessions (see `todo.txt`, rounds before 2026-09-01's third) had **no way to compile-check or playtest at all** — every session ended with an honest "[ ] could not verify" caveat. Two things changed that:

1. **A live Unreal MCP connection** (`mcp__unreal-mcp__*` tools) became available once the user had the Unreal Editor open and connected. This exposes: `EditorAppToolset` (Play-In-Editor start/stop/status, viewport/asset capture, actor/asset selection), `LogsToolset` (read the live output log, filter by category/pattern), `SceneTools` (load levels, find/query actors by class), and `ObjectTools` (read/write arbitrary UObject properties by soft-path reference, **provided the property is public** — private `UPROPERTY`s and plain C++ members are not accessible this way, confirmed the hard way, see §4).
2. **Multi-client PIE**, which the `StartPIE` MCP tool has no direct option for, was achieved by writing directly to the `ULevelEditorPlaySettings` class-default-object via `ObjectTools.set_properties` at the soft path `/Script/UnrealEd.Default__LevelEditorPlaySettings` — the same underlying settings object the editor's own "Number of Players" dropdown drives:
   ```json
   {
     "playNetMode": "PIE_ListenServer",
     "playNumberOfClients": 3,
     "runUnderOneProcess": true,
     "clientWindowWidth": 640,
     "clientWindowHeight": 480
   }
   ```
   After setting this once, subsequent `StartPIE` calls launch a 3-client listen-server session automatically.

**Known hard limits of this tooling** (worth knowing before trusting any future "playtest passed" claim from a session using it):
- No console-command execution tool exists — cvars can be searched/read/set (`SearchCVars`, `Get/SetVerbosity`), but arbitrary console commands (e.g. manually triggering `ServerTravel`, or `Live Coding.Compile`) cannot be run.
- No input-injection tool exists — UI buttons cannot be clicked programmatically, so end-to-end flows that require clicking through menus (e.g. Host Game → Lobby → kiosk interact → Settings) still need a manual playtest pass.
- No compile/build trigger tool exists — a C++ change requires the user to manually trigger a Live Coding compile, a Hot Reload, or an editor restart before a session can verify it took effect (confirmed via `LogLiveCoding`/`LogModuleManager`/UBT log entries, as in §4).
- Object introspection only reads **public** reflected properties — most of this project's own gameplay state is intentionally private (see §4's caveat), so a "no crash, no error" result is often the strongest available signal, not a full behavioral proof.

---

## 7. Compile-check & playtest log (chronological, brief)

| When | What was checked | Result |
|---|---|---|
| 2026-09-01, round 3 | Editor load (all of round 2's new C++) | Clean — no `LogCompile`/`LogBlueprint` errors, MapCheck 0/0 on `Lvl_MainMenu` and `Lvl_Game` |
| 2026-09-01, round 3 | Single-instance PIE, `Lvl_MainMenu` | Clean, zero `Logjinzza` warnings/errors |
| 2026-09-01, round 3 | Single-instance PIE, `Lvl_Game` | Clean; confirmed `StartPlay()`→`AssignRoles()` ran (correctly bailed with 1 player); confirmed pawn is `BP_FirstPersonCharacter_C`, not bare `ADefaultPawn` |
| 2026-09-01, round 4 (pre-fix) | 3-client listen-server PIE, `Lvl_Game` | **Bug found**: `AssignRoles` fired with only 1 of 3 players present (§4) |
| 2026-09-01, round 4 (post-fix) | Same 3-client test, after user's Hot Reload compile | Premature warning gone; no errors/asserts; fix trusted pending the temp debug log's positive confirmation |

---

## 8. Things only the user can do

- [ ] Register a Steamworks partner account and create a real App ID for this game (Steamworks partner site) — replace the placeholder `SteamDevAppId=480` in `Config/DefaultEngine.ini` once issued.
- [ ] Obtain and install the Vivox plugin (voice stack decision: **Vivox**, confirmed 2026-09-01). It is **not** bundled with this UE 5.8 install (checked `Engine/Plugins/Online` directly — absent). Get it directly from Vivox, or fall back to Epic Online Services' built-in Vivox-backed EOS Voice Chat (needs an EOS Product/Client on the Epic Dev Portal instead). This unblocks `UVoiceDisguiseComponent`/`UProximityVoiceComponent` (Week 6).

---

## 9. Roadmap (design doc's week numbering)

| Week | Topic | Status |
|---|---|---|
| 1 | Project scaffold | Done |
| 2 | Movement & networking polish | Partially covered as a side effect of the `DefaultPawnClass` fix (§3.6) — still needs a dedicated polish/replication-test pass |
| 3 | Lobby skeleton | Functionally covered by `UjinzzaGameInstance` + `AjinzzaLobbyGameState` + `AjinzzaRoomSettingsKiosk`, under different class names than the doc's |
| 4 | Round state machine | **Done** (§3.5) |
| 5 | Roles & disguise | **Done** (§3.6), except 2-judge mode and lobby-customization-snapshot cloning (deliberately deferred, §3.6/§5) |
| 6 | Voice system | Blocked on Vivox plugin acquisition (§8) |
| 7 | Question time & voting | Not started — `UQuestionTimeSubsystem`, `AVoteManager`, `WBP_VoteUI` |
| 8 | 1:1 interview & ghost state | Not started |
| 9 | Map blockout | Not started — 6 zones (Lobby/Self-Intro/Question/Free-Time/Interview/Evaluation) |
| 10 | Emotes & props | Not started |
| 11 | Lobby options & scaling | Partially done early (kiosk settings UI); auto mid-eval-count scaling by player count not done |
| 12+ | Sound/art pass, playtesting, Steam store page, QA | Not started |

**Still-undecided design details** flagged in the doc itself (§16), to be nailed down before or during their respective weeks: emote radial menu concept art, noise-prop balance (deferred to playtesting), 2-judge mode's vote/interview rules, and the full "candidate vote" flow for choosing the Real One.

---

## 10. File manifest (this project's own code, not engine/template boilerplate)

**Config**
- `Config/DefaultEngine.ini` — Steam OSS/NetDriver config, level defaults, `GameUserSettingsClassName`
- `Config/DefaultInput.ini` — Interact (E) action mapping
- `jinzza.uproject` — SteamSockets, AudioCapture, Synthesis plugins enabled

**Core game framework** (`Source/jinzza/`)
- `jinzza.h/.cpp` — `Logjinzza` log category
- `jinzza.Build.cs` — module dependencies (`AudioCapture`, online subsystem modules)
- `jinzzaMatchSettings.h` — `FJinzzaMatchSettings` (room name, max players, judge/vote count, phase speed, role-assign method)
- `jinzzaRoundTypes.h` — `EJinzzaRoundPhase`, `EJinzzaPartyRole`, `EJinzzaFaceType`, `EJinzzaVoiceFilter`
- `jinzzaGameInstance.h/.cpp` — Steam session lifecycle, rich presence, live session updates (§3.1)
- `jinzzaGameUserSettings.h/.cpp` — Graphics/Audio/Controls/Gameplay/Microphone settings (§3.4)
- `jinzzaCharacter.h/.cpp` — base character; disguise component added; `DoAim` wired to sensitivity/invert-Y
- `jinzzaPlayerController.h/.cpp` — `BuildRuntimeMappingContext` (key rebinding, §3.4)
- `jinzzaDisguiseComponent.h/.cpp` — third-person face-material disguise (§3.6)

**Lobby** (`Lvl_Lobby`)
- `jinzzaLobbyGameMode.h/.cpp`
- `jinzzaLobbyGameState.h/.cpp`
- `jinzzaLobbyPlayerController.h/.cpp` — kiosk-proximity polling
- `jinzzaRoomSettingsKiosk.h/.cpp` — physical in-world settings kiosk (§3.3)

**Match gameplay** (`Lvl_Game`)
- `jinzzaGameGameMode.h/.cpp` — round driver, `AssignRoles()`, the PostLogin fix (§3.6, §4)
- `jinzzaGameGameState.h/.cpp` — replicated round-phase state (§3.5)
- `jinzzaGamePlayerController.h/.cpp` — `Client_ReceiveRoleAssignment` RPC (§3.6)
- `jinzzaPartyPlayerState.h/.cpp` — per-player round state, non-replicated `ServerRole` (§3.6)
- `jinzzaRoundPhaseSubsystem.h/.cpp` — the 8-phase state machine (§3.5)

**UI** (`Source/jinzza/UI/`)
- `jinzzaUIStyle.h/.cpp` — shared Slate styling (§3.2)
- `jinzzaMainMenuWidget.h/.cpp`
- `jinzzaLobbyWidget.h/.cpp` — Start Match, Invite Friends
- `jinzzaRoomSettingsWidget.h/.cpp` — kiosk panel contents
- `jinzzaSettingsWidget.h/.cpp` — 4-tab settings screen
- `jinzzaGameEndWidget.h/.cpp`

**Content** — everything moved under `Content/JINZZA/...` (FirstPerson, Variant_Horror, Variant_Shooter, Characters, Weapons, LevelPrototyping, Input); `Collections/` and `Developers/` left in place (editor-managed).

---

*End of report. For anything that has happened since this was generated, `todo.txt` is authoritative.*
