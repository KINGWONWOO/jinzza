# JINZZA (진짜를 찾아라 — "Find the Real One")

A solo-developed Unreal Engine 5.8 C++ social-deduction party game.

## What this is

Each round, one player is secretly the **Real One**, several others are **Imitators** disguised to look and sound identical to them (same face variant, same voice filter), and one player is the **Judge**, who has to figure out who the Real One is through free-time observation, a written Q&A round, and 1:1 interviews. The core design principle is **정보 비대칭이 곧 서스펜스다** — "information asymmetry is the suspense": the Judge must never be able to infer anything from replicated network data, so role knowledge is leaked to each client only via targeted RPCs, never broadcast state.

Multiplayer is Steam-session-based and **invite-only by design** — no public matchmaking or room browser, since this is a friends-only party game.

Full design doc: `docs/game.docx` / `docs/game_extracted.txt`.

## Development log

Dated, chronological session history. `todo.txt` is the full detailed log this is condensed from; `docs/PROJECT_STATUS.md` reorganizes the same history by system instead of by date.

- **2026-08-26** — UE 5.8 C++ project scaffolded from the First Person template. Enabled AudioCapture/Synthesis plugins. Added Steam OnlineSubsystem config with Valve's public test App ID (480) as a placeholder. Reorganized all content into `Content/JINZZA/`.
- **2026-08-30** — Pulled lobby/settings work forward early: Host Game now creates a Steam session and travels straight to the lobby (no pre-create setup screen); joining is invite-only via Steam overlay. Added the in-world **Room Settings kiosk** (walk up, press E). Built a full 4-tab Settings screen (Graphics/Audio/Controls/Gameplay) mirroring Epic's Lyra architecture. Added a shared `JinzzaUI` Slate styling namespace. Fixed a real networking gap: game traffic wasn't routed through Steam's P2P relay (missing `SteamSocketsNetDriver` + SteamSockets plugin) — fixed by matching config from an earlier reference project.
- **2026-09-01, round 1** — Closed remaining gaps against the Steam session/settings adaptation plan: live session updates when room settings change (`UpdateLiveSessionSettings`), Steam Rich Presence, and push-to-talk/open-mic + microphone device selection in Settings.
- **2026-09-01, round 2** — Built the round-phase state machine and role/disguise assignment (first slice of actual match gameplay): the 8-phase round flow (`RoleAssignment → ... → RoundComplete`), replicated `AjinzzaGameGameState`, `AjinzzaPartyPlayerState` with a deliberately non-replicated server-only role field, `AjinzzaDisguiseComponent` for face-material disguise, and `AssignRoles()` (Fisher-Yates role shuffle + targeted per-player RPCs).
- **2026-09-01, round 3** — Editor restart unlocked a live Unreal MCP connection for compile-checking and PIE playtesting. Verified round 2's changes compiled clean and ran without errors in single-instance PIE.
- **2026-09-01, round 4** — Set up 3-client listen-server PIE and found a real bug: role assignment ran synchronously on `StartPlay()`, racing remote player connections, so only whichever player connected first ever got a role. Fixed with a `PostLogin()` + 1.5s debounce (`TryStartRound`) instead of a fixed head-count. Verified fixed after the user's Hot Reload compile.
- **2026-09-02** — Committed and pushed the full accumulated working tree (865 files: all of the above source, the moved/added `Content/JINZZA/` assets, and docs) to `origin/main` for the first time since the initial scaffold commit.

## What's being built next

Roadmap by the design doc's week numbering (see `docs/PROJECT_STATUS.md` §9 for the full table):

| Week | Topic | Status |
|---|---|---|
| 1–3 | Scaffold, movement/networking polish, lobby skeleton | Done / functionally covered |
| 4 | Round state machine | Done |
| 5 | Roles & disguise | Done, except 2-judge mode and lobby-customization-snapshot cloning (deliberately deferred — undecided in the design doc) |
| 6 | Voice system | **Blocked** — needs the Vivox plugin obtained/installed (decision made: Vivox) |
| 7 | Question time & voting | Not started |
| 8 | 1:1 interview & ghost state | Not started |
| 9 | Map blockout (6 zones) | Not started |
| 10 | Emotes & noise props | Not started |
| 11 | Lobby options & scaling | Partially done (kiosk settings UI); auto mid-eval scaling by player count not done |
| 12+ | Sound/art pass, playtesting, Steam store page, QA | Not started |

**Things only the user can do:**
- Register a Steamworks partner account and issue a real App ID (replace the placeholder `SteamDevAppId=480` in `Config/DefaultEngine.ini`).
- Obtain and install the Vivox plugin (or fall back to EOS Voice Chat) to unblock the Week 6 voice system.

**Still-undecided design details** (design doc §16, to be settled before/during their build weeks): emote radial menu concept art, noise-prop balance (deferred to playtesting), 2-judge mode's vote/interview rules, and the full "candidate vote" flow for choosing the Real One.

## More detail

- `todo.txt` — full chronological session-by-session log, including exact verification steps and caveats.
- `docs/PROJECT_STATUS.md` — the same history reorganized by system ("what exists and how it works").
- `docs/game.docx` / `docs/game_extracted.txt` — design doc.
- `docs/steam_session_and_settings_reference.md` — Steam/settings adaptation-plan cross-check.
