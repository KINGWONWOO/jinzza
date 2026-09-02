# Reference Investigation: Steam Sessions & Settings System (from Unreal_Game_Noob) — Implementation Brief for "Find the Real One"

## How to use this document

You (the Claude session reading this) are being asked to implement the **Steam session (create/invite/accept)** and **settings menu (graphics/audio/gameplay/general)** systems for a Korean-language 3D social-deduction party game, working title **"진짜를 찾아라" ("Find the Real One")**, built in **Unreal Engine 5.8, C++, first-person, Steam release**. You were not part of the conversation where this game was designed, so this document restates the facts you need and then hands you a second, separate project's real implementation as a reference to adapt — not to copy verbatim.

If a file named `게임기획_요약본.docx` is also provided to you in this same handoff, treat it as the authoritative design document — it has the full round structure, roles, map zones, and a "C++ 클래스 목록" (C++ class list) section that already names `USteamLobbySubsystem`, `UPartyGameInstance`, `UDisguiseComponent`, `UVoiceDisguiseComponent`, etc. Everything below is written to plug into that class list. If that docx is not provided, ask the user for it before starting — the class names and section numbers referenced below come from it.

### Project facts you need (restated for self-containment)

- Engine: Unreal Engine 5.8, C++ project, **First Person** template, Steam release via Steamworks.
- Player count: **4–12, adjustable in the lobby**, not fixed. Roles are always 1 "진짜"(the Real One) + 1 "판정단"(Judge) + the rest "모방자"(Imitators); the judge and imitators count as normal Steam session slots.
- Voice: **near-field/proximity voice via Vivox** (not Steam's baseline VoIP) — this matters because the reference project below uses plain Steam voice, which is a materially different system.
- Map: a single persistent level, 6 zones, moved between via teleport (no level streaming/travel between phases) — see the docx section "5. 맵 구역 설계" if provided.
- A round has ~8 phases with a server-authoritative timer per phase (see docx section "6. 라운드 구조").
- Rich Presence should show a friend which phase they're currently in (this was called out in the original design brainstorm as a nice-to-have; the reference project below gives you a concrete way to do this).

## 1. Source and method (so you can judge how much to trust each claim below)

The reference project is a public solo-developer portfolio repo: `https://github.com/KINGWONWOO/Unreal_Game_Noob` (UE 5.6, "NoobGame"). Its C++ source was read directly (100% reliable). Its Blueprint assets (`.uasset`) are stored via Git LFS; a plain GitHub ZIP download only contains LFS *pointer* files, not the real binaries, so the actual binaries were pulled separately via the LFS batch API and their strings extracted with `strings`. That means: **every function name, variable name, plugin name, and widget/button name quoted below is confirmed to exist in the Blueprint** (it's a real identifier from the binary), but **the exact node-to-node wiring inside each graph could not be verified** — only someone opening the file in the Unreal Editor can see the literal wire connections. Treat the *architecture and function list* as reliable, and the *exact sequencing* as "best inference from the evidence," clearly marked where it's inference vs. fact.

## 2. Confirmed plugin/config setup for Steam in the reference project

Two free, well-known community plugins (not raw `IOnlineSubsystem` C++ code) do the heavy lifting:

- **Advanced Sessions** (Joshua Statzer / "Mordentral", v5.5) — GitHub: `mordentral/AdvancedSessionsPlugin`. Adds Blueprint-callable nodes for session create/find/join, session settings, and friends-list operations.
- **Advanced Steam Sessions** (same author, v5.4) — GitHub: `mordentral/AdvancedSteamSessionsPlugin`. Requires Advanced Sessions; adds Steam-specific nodes (friend avatar, richer presence data).

Plus the standard engine modules: `OnlineSubsystem`, `OnlineSubsystemSteam`, `OnlineSubsystemUtils`, `SteamSockets`, `SteamController`.

Relevant `DefaultEngine.ini` block (copy the shape, not necessarily every value):

```ini
[OnlineSubsystem]
DefaultPlatformService=Steam

[/Script/Engine.GameEngine]
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

[OnlineSubsystemSteam]
bEnabled=true
SteamDevAppId=480
bInitServerOnClient=true
GameServerQueryPort=27015
bAllowP2PPacketRelay=true
P2PConnectionTimeout=90.0
P2PCleanupTimeout=60.0
```

`SteamSocketsNetDriver` is the default net driver (P2P with NAT traversal via Steam relay), falling back to `IpNetDriver`. `SteamDevAppId=480` is Valve's public test app ("Spacewar") — **you must replace this with the real Steam App ID once one is registered**; `steam_appid.txt` in the project root needs the same ID for local testing before the game is live on Steam.

**Important divergence for our project**: the reference project leaves `[Voice] bEnabled=true`, which turns on Steam/engine's *baseline, non-spatial* VoIP. Our project needs **Vivox** for actual proximity/near-field voice (loud in person, muffled or absent through walls, the isolated 1-on-1 interview channel, etc.). Do not copy the `[Voice]` section as-is — Vivox is configured separately (its own plugin + `VivoxCore` initialization) and can coexist with `SteamSockets` as the net driver; they solve different problems (voice transport vs. game-state transport).

## 3. Steam session: Host / Create Session (confirmed)

### 3.1 C++ layer — the menu architecture the reference project uses

This is a very standard, well-tested pattern (it matches the structure taught in several popular UE multiplayer courses) and is worth adopting close to as-is:

```
IMenuInterface                          (pure interface, no implementation)
├─ Host()
├─ Join(const FString& Address)
├─ LoadMainMenu()

UMenuWidget : UUserWidget                (base class for every full-screen menu widget)
├─ Setup()     — AddToViewport() + switches PlayerController to Input Mode UI Only, shows cursor
├─ Teardown()  — RemoveFromParent() + switches back to Input Mode Game Only, hides cursor
├─ SetMenuInterface(IMenuInterface*)      — the widget is handed a pointer back to the GameInstance

UMainMenu : UMenuWidget                  (C++ parent of the WBP_MainMenu Blueprint)
├─ BindWidget: HostButton, SettingButton, QuitButton
├─ only QuitButton is bound in C++ (calls ConsoleCommand("quit")); Host/Setting are wired in the Blueprint graph

UNoobGameInstance : UGameInstance, public IMenuInterface
├─ Host()          → Teardown() the menu widget, then World->ServerTravel("/Game/Levels/Lobby?listen")
├─ Join(Address)   → PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute)
├─ LoadMainMenu()  → ClientTravel("/Game/MenuSystem/MainMenu", TRAVEL_Absolute)
├─ InGameLoadMenu()→ creates the in-game pause menu widget, calls SetMenuInterface(this)
└─ loading screen: binds FCoreUObjectDelegates::PreLoadMap / PostLoadMapWithWorld to show/hide a loading widget
```

Notice `Host()` itself never touches the Steam Session API — it only does `ServerTravel(...?listen)`. The *session* (the thing a friend can search for or be invited into) is created separately, in the Blueprint layer, by an Advanced Sessions node.

### 3.2 Blueprint layer — where "Create Session" actually happens

In `WBP_MainMenu`'s event graph: the `HostButton` click is wired to the **`Create Session`** node (an Advanced Sessions Blueprint function), and a `FriendlyName` variable exists that is almost certainly fed into it as the session's display name. The inferred sequence is: `Create Session` → on success, call `IMenuInterface::Host()` (i.e., the GameInstance) → which does the actual `ServerTravel`.

**Adapt for our project**: `Create Session` needs a `Public Connections` / max-players parameter driven by the lobby's chosen player count (4–12, from the lobby settings UI in docx section "9. 로비 사전 설정 옵션"), not a fixed number. Advanced Sessions' Create Session node exposes this. You'll also want to pack the lobby's pre-round settings (question-time cycle count, judge count, etc.) into the session's searchable settings so a session browser (if you build one — see the gap below) could filter on them, though for a friends-only party game this is optional.

## 4. Steam friend invite (confirmed)

Two paired widgets:

**`WBP_InvitePannel`** (the list container):
- Buttons: `BT_RefreshFriendList`, `BT_ExiteButton`.
- Declares a custom delegate `BlueprintGetFriendsListDelegate` and calls **`GetAndStoreFriendsList`** (an Advanced Sessions/Advanced Friends Library function) to pull the Steam friends list. Two branches exist for success ("friends list successfully retrieved") and failure ("error retrieving the friends list").
- Iterates the result array (of `BPFriendInfo` structs) with `Array_Length`, and for each entry does `Create` + `AddChild` to spawn a `WBP_FriendInfo` child row — a standard "list = loop-spawn child widgets" pattern.

**`WBP_FriendInfo`** (one row per friend):
- Button `BT_Invite` → calls **`SendSessionInviteToFriend`** (Advanced Sessions) — this is the actual invite send.
- Calls **`GetSteamFriendAvatar`** (Advanced Steam Sessions) to display the friend's avatar.
- Holds `BPUniqueNetId` and `BPFriendPresenceInfo` (with `EBPOnlinePresenceState`, e.g. `Offline`) per friend, so this one widget encapsulates one friend's id + online status + presence text.

**Function cheat-sheet for your implementation:**

| Purpose | Function (Advanced Sessions / Advanced Steam Sessions) |
|---|---|
| Get the Steam friends list | `GetAndStoreFriendsList` (async; success branch returns a `BPFriendInfo` array, failure branch returns an error) |
| Send an invite to one friend | `SendSessionInviteToFriend` |
| Get a friend's avatar texture | `GetSteamFriendAvatar` |
| Read a friend's online/presence state | fields on `BPFriendPresenceInfo` / the `EBPOnlinePresenceState` enum |
| **(Not confirmed in the repo, but exists in the plugin — use for the docx's "Rich Presence shows current phase" request)** | Advanced Steam Sessions exposes a "Set Rich Presence" style node; use it to publish the local player's current round phase as a presence string other players' friends lists can show |

## 5. GAP — Find/Join Session and invite-accept are NOT implemented in the reference project

This is the most important thing to flag before you start coding: I checked every downloaded Blueprint and all of the C++ source, and **none of the following exist anywhere in the reference repo**:

- Any call to `Find Sessions` / `FindSessions`.
- Any call to `Join Session` / `JoinSession`.
- Any binding of `On Session User Invite Accepted` (or any other "invite was accepted, auto-join" delegate).

What *does* exist is UI scaffolding that implies this was planned: a `FindButton` and `JoinButton` (with hover animations) on the in-game menu widget, and a `WBP_ServerRow` widget with `FriendlyName`/`HostUser` fields clearly meant to represent one row of a session-browser list. None of it is wired to real session-search/join logic as far as the evidence shows. The most likely real behavior today is that `JoinButton` just feeds a typed IP/address string into the existing C++ `IMenuInterface::Join(Address)` → `ClientTravel`, i.e., manual direct-connect, not session search.

**You must design and build this yourselves.** Advanced Sessions provides the nodes to do it properly:

- **`Find Sessions`** — search for open sessions (of friends, or public); bind the result array to a session-row widget list (`WBP_ServerRow`'s pattern is a fine template).
- **`Join Session`** — join a specific found/selected session.
- **`Bind to On Session User Invite Accepted`** — fires when the player accepted a Steam overlay invite (which may relaunch or foreground the game); your handler should call `Join Session` on the session referenced in the event automatically, so accepting an invite in the Steam overlay actually drops the player into the lobby without extra clicks. This is the piece that makes "초대 수락" ("accept invite") actually work end-to-end — build it explicitly, it will not happen by itself just from enabling `OnlineSubsystemSteam`.

## 6. Settings menu system (confirmed)

### 6.1 Structure

```
WBP_Option                              (the settings window container; opened from the main menu's "설정" button)
├─ Tab buttons: BTN_GraphicTab / BTN_AudioTap / BTN_ManipualteTab / BTN_ETCTab   (typos are in the original, don't copy them)
├─ WS_MenuSwitcher (a WidgetSwitcher)   → SetActiveWidgetIndex switches the visible tab
│    ├─ children confirmed: WBP_Graphic, WBP_Audio, WBP_GamePlay
├─ ApplyButton / CancelButton
└─ each tab widget exposes a custom "Set All <Category> Setting(s)" function that Apply calls

WBP_InOption                            (in-game ESC/pause menu)
├─ BTN_Back, BTN_Setting (opens WBP_Option), BTN_Quit
└─ BTN_Quit calls QuitGame(EQuitPreference) — supports a "quit to background" style preference, not just an instant kill

BP_SettingsSaveGame : USaveGame          (a single save object holding every tab's values together)
```

Only three tab child widgets were confirmed by name inside `WBP_Option` (Graphic/Audio/GamePlay); a fourth Blueprint, `WBP_GamePlay1`, exists as a near-duplicate of `WBP_GamePlay` but additionally has a language/culture setting — it's ambiguous from the evidence whether `WBP_GamePlay1` is the actual "ETC/일반" tab or simply a newer iteration that superseded `WBP_GamePlay`. Don't take the exact tab-to-widget mapping as gospel; take the **field list and function list** as gospel.

### 6.2 Graphics tab — confirmed functions (all are stock `UGameUserSettings` calls)

| Setting | Function | UI pattern |
|---|---|---|
| Resolution | `SetScreenResolution` | a "◀ value ▶" index stepper, not a dropdown — `Resolution+`/`Resolution-` buttons with `Get_Resolution_Text` |
| Window mode | `SetFullscreenMode` (`EWindowMode`: Fullscreen / Windowed / WindowedFullscreen) | stepper |
| VSync | `SetVSyncEnabled` | stepper |
| Frame rate cap | `SetFrameRateLimit` | stepper |
| Anti-aliasing quality | `SetAntiAliasingQuality` | stepper |
| Shading quality | `SetShadingQuality` | stepper |
| Shadow quality | `SetShadowQuality` | stepper |
| Texture quality | `SetTextureQuality` | stepper |

Two separate apply functions exist — `ApplyResolutionSettings` and `ApplyNonResolutionSettings` — almost certainly because resolution changes need a "confirm within N seconds or revert" safety flow (`ConfirmVideoMode`-style) while the others can apply immediately.

### 6.3 Audio tab — confirmed functions

Standard UE Sound Class + Sound Mix architecture:

- 4 Sound Class assets: `SC_Master`, `SC_Music`, `SC_SFX`, `SC_Voice`.
- 4 sliders: `SD_MasterVolume`, `SD_MusicVolume`, `SD_SFXVolume`, `SD_VoiceVolume`, each with a text label.
- A custom function ("Set Sound Class with SlideBar") calls `SetSoundMixClassOverride` (targeting the matching Sound Class) plus `SetBaseSoundMix`.
- Each slider binds all three of `OnMouseCaptureBegin` / `OnMouseCaptureEnd` / `OnFloatValueChanged` — consistent with "preview live while dragging, commit on release."

**Adapt for our project**: our game already plans a **`SC_Voice`-equivalent channel for the disguised/pitch-shifted proximity voice** — reuse this exact Sound Class + slider + `SetSoundMixClassOverride` pattern for that channel, and add it as a 5th slider (or repurpose `SC_Voice` directly) rather than inventing a separate volume system.

### 6.4 Gameplay/manipulation tab — confirmed functions

- Camera sensitivity: a slider run through `MapRangeClamped` to convert the 0–100 UI range into the actual turn-rate value applied to the character.
- Axis inversion: `CB_CameraXReverse`, `CB_CameraYReverse` checkboxes.
- Language: only in `WBP_GamePlay1` — a `CB_Language` control and `SetCurrentCulture` (engine localization function); `CurrentLanguage` variable; the string `"Korean"` appears in the asset.
- Save/load calls (`CreateSaveGameObject`, `DoesSaveGameExist`, `LoadGameFromSlot`, `SaveGameToSlot`) are made **directly from the tab widget**, not routed through the GameInstance.

### 6.5 The single SaveGame object — confirmed field list

```
CameraSensitivity   (float)
CameraXreverse      (bool)
CameraYreverse      (bool)
CurrentLanguage     (string/culture code, e.g. "Korean")
FrameRate           (float/int)
MasterVolume        (float)
MusicVolume         (float)
SFXVolume           (float)
VoiceVolume         (float)
ResolutionInex      (int — sic, original typo for "Resolution Index")
ShadingQ            (int — Shading Quality)
ShadowQ             (int — Shadow Quality)
TextureQ            (int — Texture Quality)
VSyncEnabled        (bool)
WindowMode          (EWindowMode enum)
```

All settings across all tabs live in one flat `USaveGame` object, not one per tab.

## 7. What must change for "Find the Real One" — adaptation plan

Go through this list in order; it maps each reference-project piece to a decision or a concrete change for our project.

1. **Fix the typos, don't inherit them.** `ResolutionInex`, `AudioTap`, `ManipualteTab` are typos in the source project. Name your equivalents cleanly (`ResolutionIndex`, `BTN_AudioTab`, `BTN_ManipulateTab`, etc.).
2. **Player count is 4–12 and adjustable — the reference project's Host flow assumes a fixed setup.** When you call Advanced Sessions' `Create Session`, drive `Public Connections` from the lobby's current player-count setting (docx section 9), and recreate/update the session if the host changes the count before starting (check whether Advanced Sessions supports updating an existing session's settings, or whether you need to destroy and recreate it).
3. **Build Find/Join/Accept-invite from scratch — do not assume they exist anywhere to copy from.** Per Section 5 above, this is a hard gap in the reference project. Use `Find Sessions` + `Join Session` + `Bind to On Session User Invite Accepted`. This is the single most important net-new piece of work relative to the reference project.
4. **Do not reuse `[Voice] bEnabled=true` / Steam's baseline VoIP.** Our proximity/disguise voice runs on Vivox, layered on top of (not instead of) the `SteamSockets` net driver for the game-state connection. Keep the `SteamSockets`/`OnlineSubsystemSteam` config, drop the baseline `[Voice]` section, and configure Vivox separately per its own plugin docs.
5. **Extend Rich Presence to publish the current round phase**, per the design brief's "Rich Presence(친구가 어느 페이즈인지 표시)" requirement. Advanced Steam Sessions' presence-related nodes are the mechanism; call it from wherever `URoundPhaseSubsystem` transitions phases so a friend's Steam friends-list entry updates live.
6. **Merge the settings SaveGame's shape into our own `UPartyGameInstance`-managed settings, but keep the four-category split.** Section 8 of the docx (구현 기능 목록) and the lobby options already imply Graphics/Audio/Gameplay/General categories map cleanly onto this reference's four tabs — reuse that split, not the exact field list. Add fields the reference project has no reason to need: a push-to-talk vs. open-mic toggle and a microphone device selector (this game is voice-driven and near-field; letting a player choose push-to-talk matters a lot more here than in a single-player-feel portfolio project), and consider a captions/subtitle toggle for the disguised-voice channel (accessibility matters more when voices are deliberately pitch-shifted and hard to parse).
7. **Prefer C++ over heavy Blueprint logic for session/settings code**, unlike the reference project (which is Blueprint-heavy throughout, reasonable for a learning portfolio but not ideal for a project you intend to actually ship and maintain). The docx's own C++ class list already names `USteamLobbySubsystem` and `UPartyGameInstance` — implement the Create/Find/Join/Invite/Accept logic and the settings-apply logic as C++ functions on those classes, and keep Blueprint only for the widget visuals and simple event wiring (button → C++ function call), matching how `UMenuWidget`/`UMainMenu` in the reference project keep only the trivial `QuitPressed` in C++ while everything is exposed as `BlueprintCallable`.
8. **Decide the graphics-tab control style deliberately.** The reference project uses "◀ value ▶" steppers instead of dropdowns for every graphics option. Either is fine technically; a stepper reads a little more "console-friendly" while a dropdown/ComboBox is a little more standard for PC. This is a UX call for the team, not a technical constraint — flagging it so it's a decision, not an accident.

## 8. Other things worth borrowing beyond what was asked for

These weren't the original ask (settings + Steam sessions) but showed up during the investigation and are directly relevant to this project:

### 8.1 The `FruitGame` minigame's replication pattern — a near-perfect template for the round/role system

`FruitGame` (a 1v1 turn-based deduction minigame in the same repo) is cleanly built around exactly the primitives our round/role system needs:

- `AFruitGameState` replicates `CurrentGamePhase` (an enum) via `ReplicatedUsing = OnRep_GamePhase`, and fires a `BlueprintAssignable` multicast delegate `OnGamePhaseChanged` from inside `OnRep_GamePhase()` so every Blueprint UI widget can just bind to one event instead of polling. **This is exactly the shape `APartyGameState`'s phase/timer replication should take** — replicate the current phase enum with a RepNotify, broadcast a delegate from the RepNotify, and have `WBP_PhaseTimer` and friends bind to it once.
- `AFruitPlayerState` keeps the hidden information (`SecretAnswers`, a `TArray<EFruitType>`) as a plain `Replicated` property, with server-only setters (`SetSecretAnswers_Server`) — i.e., replication doesn't have to mean "everyone sees it," visibility rules still apply per-connection, but the *pattern* of "server-authoritative hidden data lives on PlayerState" maps directly onto where "who the 진짜 is" and each imitator's knowledge state should live in `APartyPlayerState`.
- `AFruitPlayerController` cleanly separates four RPC directions that our round system needs too: `BlueprintCallable` local-only functions the UI calls → matching `Server, Reliable, WithValidation` RPCs that do the actual state change → `Client, Reliable` RPCs for private per-player notifications (e.g., "here is the answer only you should see") → and `BlueprintAssignable` delegates the UI binds to for the client-side result. This is the same shape your `UQuestionTimeSubsystem`/`AVoteManager` will want: server collects everyone's submission privately, then reveals via a controlled broadcast.

### 8.2 `ChangeOpenUICount` — a one-line pattern worth stealing outright

`ANoobPlayerController::ChangeOpenUICount(bool OpenUI)` is a tiny reference-counter (`OpenUICount++`/`FMath::Max(0, OpenUICount - 1)`) that widgets call when they open/close, so input mode (mouse cursor shown, UI-only vs. game-only input) is only toggled back to "game" once *no* widget is still open — instead of every widget's Teardown() naively flipping input mode back on and accidentally undoing another still-open widget's UI mode. Our game will very plausibly have multiple simultaneous overlays (phase timer + vote UI + ghost HUD + emote radial menu), so this exact one-line counter is worth lifting into `APartyPlayerController` as-is.

### 8.3 The GameInstance-level loading-screen delegate pattern

`UNoobGameInstance` binds `FCoreUObjectDelegates::PreLoadMap` / `PostLoadMapWithWorld` once in `Init()` to show/hide a loading widget automatically around any map load, rather than every call site remembering to show/hide it manually. Even though our design deliberately avoids level loads *between phases* (teleport-only, per the docx), this pattern is still exactly right for the one map load we do have — the initial `Lobby?listen` / joining-a-session transition — so `UPartyGameInstance` should bind the same two delegates the same way for that moment.

### 8.4 Steam friend presence data doubles as a "who's already playing" signal

Since `BPFriendPresenceInfo`/`EBPOnlinePresenceState` are already being read for the invite panel, the same data can drive a "this friend is already in a game, invite is disabled/relabeled" state on `BT_Invite` in `WBP_FriendInfo`'s equivalent, instead of only using it for the avatar/online-dot. Small UX win, same data you're already fetching.
