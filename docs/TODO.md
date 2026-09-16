# JINZZA — Action To-Do List

**Generated**: 2026-09-15, by Claude Code, from a live pass over the running editor (MCP) plus the
full `Source/` tree, `todo.txt`, and `README.md`. This file is the current actionable punch list —
`todo.txt` stays the full chronological session log, `docs/PROJECT_STATUS.md` is a stale
(2026-09-01) narrative snapshot. Re-generate/update this file rather than letting it drift the way
`PROJECT_STATUS.md` did.

---

## 0b. Fixed this round (2026-09-16) — cartoon post-process, soft sun, ambient dust particles

- [x] **Cartoon-style color grading on `PostProcessVolume_0`** (Lvl_Lobby, Unbound/global): added
  saturation (+20%), contrast (+12%), and a warm-highlights/cool-shadows split tone (classic
  stylized-sunny-day look), plus a bloom bump (0.675→0.9) for a glowier feel. Left your own
  exposure override (`AutoExposureBias≈2.73`, `AutoExposureMethod=Basic`) untouched - only added
  fields, verified nothing else got reset. **Caveat: I have no reliable way to screenshot PIE, so
  none of this is visually confirmed** - these are reasonable starting values, not a final look.
  Open the volume in the editor and tell me which direction to push (more/less saturated, warmer/
  cooler, punchier/flatter) and I'll dial it in numerically.
- [x] **Patchy dappled tree-shadow fix**: `DirectionalLight_0` was using the real sun's actual
  angular size (`LightSourceAngle=0.54°`), which is why light through the outdoor trees was
  producing sharp, high-frequency leaf-shaped light patches instead of a soft wash - realistic hard
  sun angle + tiny leaf gaps = harsh dapple. Increased `LightSourceAngle` to 8° and
  `LightSourceSoftAngle` to 4° (a much bigger, softer "sun disc"), which blurs shadow penumbras
  broadly - this also nudges toward the cartoon look from the point above. Doesn't persist-conflict
  with the day/night system (`ApplyTimeOfDayVisuals` only touches rotation/intensity/color, never
  these angle fields).
- [x] **Added the ambient dust Niagara effect from the NoobGame reference project**: migrated
  `NS_Dust` + its material `M_DustImage` + texture `DustImage` from
  `Unreal_Game_Noob/Content/Niagara/Dust/` (a real, playable binary checkout - not an LFS pointer)
  into this project. Had to land them at `/Game/Niagara/Dust/...` (not the usual `/Game/Noob/...`
  convention) because `NS_Dust`'s internal material reference is a baked absolute path,
  `/Game/Niagara/Dust/M_DustImage` - moved to match rather than patch the reference. Verified
  clean: dependency chain resolves (`NS_Dust → M_DustImage → DustImage`), compiled with no errors
  in the log. Placed one instance (`NiagaraActor_0`, in a new `VFX` outliner folder) in `Lvl_Lobby`
  at `(-200, 0, 700)` at 15x scale to spread across the room's now-large scale.
  **Caveat: placement/scale is a guess** - I sized it to roughly cover the room based on other
  props' scale factors, but never saw it render (no Niagara asset-thumbnail support, no working PIE
  screenshot). Move/rescale the `NiagaraActor_0` actor directly in the editor once you can see it.
- [ ] **One harmless leftover**: `NS_Dust`'s graph has a `PlayAudio` module referencing
  `/Game/Audio/Sounds/JumpScare/Thump1003` (a jump-scare stinger - clearly an unrelated leftover
  from whatever template this dust effect was built from in the original project). That asset
  doesn't exist in this project, so it logs a one-time "dependent package ... not available"
  load warning and the module just doesn't play anything - not fatal, but worth deleting that
  module in the Niagara editor at some point since there's no tool in this session that can edit a
  Niagara graph remotely.

**Action needed**: nothing to compile (all content edits). Open Lvl_Lobby, eyeball the dust
placement/density and the color grading, and tell me what to adjust - I can't see either right now.

---

## 0a. Fixed this round (2026-09-15, continued again) — seal size, dark lobby, floating/buried props

- [x] **Grew the character to match**: after doubling `SealMesh`'s visual scale (2x), also doubled
  the collision capsule (radius 45→90, half-height 47→94) and the first-person eye height (37→74)
  so collision and camera match the bigger visual body. Content-only (Blueprint property) change,
  no compile needed - already live. Caught and fixed a real gotcha mid-edit: `UCapsuleComponent`
  clamps `CapsuleRadius` to `CapsuleHalfHeight`, so setting both in one call silently clamped the
  radius back down until half-height was applied first.
- [x] **"Seal not visible" - verified NOT a new bug**: re-ran a live 2-client PIE test and checked
  `SealMesh` on both spawned pawns directly - both correctly configured
  (`bHiddenInGame=false, bOwnerNoSee=true`, correct `SM_Seal` mesh). This is the same by-design
  `bOwnerNoSee` behavior as before (you never see your own third-person body, only other
  players/spectators do) - most likely being re-triggered by testing solo in PIE. If a *second*
  real player genuinely can't see you either, that's a different, new report - let me know
  specifically what you saw and how (solo PIE vs. two real clients).
- [x] **Lobby lighting never actually initializes**: found a real bug in `AjinzzaLobbyGameState` -
  `ApplyTimeOfDayVisuals()` (which sets `DirectionalLight_0`/`SkyLight_0` and recaptures the
  SkyLight's cubemap) was only ever called from `CycleTimeOfDay()` (clock interact) or
  `OnRep_TimeOfDay()` - never on level start. Since `TimeOfDay` defaults to `Day` and nothing
  changes it in a normal session, `OnRep_TimeOfDay` never fires (the value never changes from its
  default), so the SkyLight's captured cubemap was never refreshed and the "Day" preset was never
  positively *guaranteed* applied - the room was just running on whatever raw values were last
  saved in the level. Added a `BeginPlay()` override that calls `ApplyTimeOfDayVisuals()`
  unconditionally on every instance (server + each client) so this self-initializes correctly.
  **Caveat**: the currently-saved raw values already numerically match the "Day" preset
  (`SunIntensity=10`), so this fix mainly guarantees the SkyLight capture is fresh - I could not
  visually confirm brightness through available tooling (no reliable in-PIE screenshot). If it's
  still too dark after compiling, tell me and I'll increase `SunIntensity`/`SkyIntensity` in
  `jinzzaLobbyGameState.cpp`'s `GetPreset()` - didn't want to blind-guess numbers I can't see.
- [x] **Found and fixed 2 concrete floating/buried props** in `Lvl_Lobby` (verified via
  `get_actor_bounds` against the room's established floor level, Z≈-38.5, matching every other
  correctly-placed grounded prop): the door (`NoobProp_7_wooden+door+3d+model`) was sunk ~9 units
  into the floor; the stone fireplace (`NoobProp_65_stone+fireplace+3d+model2`) was floating at
  Z≈2530-5175 - literally up near/through the ceiling, nowhere near the floor. Both repositioned to
  sit correctly on the floor (fireplace now spans floor-to-ceiling, which fits a "stone fireplace"
  prop). Pure level edits, no compile needed - already saved.
- [ ] **Did NOT do a full audit of all ~179 `NoobProp_*` actors** in `Lvl_Lobby` - spot-checked
  about a dozen; found these 2 clearly wrong (using each mesh's own local bounds to compute the
  correct floor-touching position, not by eyeballing) plus one ambiguous case (`NoobProp_61_wooden+
  crate+3d+model8` sits at Z≈552 - could be legitimately stacked on a shelf/table, or could be
  another floating bug; didn't have a way to tell without more checks). If you spot more
  floating/buried props, name them and I'll fix precisely the same way; otherwise say so and I'll
  do the full sweep.

**Action needed**: compile `jinzzaLobbyGameState.h/.cpp` (only new C++ this round). Everything else
here is already live.

---

## 0. Fixed this round (2026-09-15, continued) — "characters not appearing in the lobby" + emote key

- [x] **Found the real cause of "characters not appearing" reports**: it was never the seal mesh
  itself (verified live via PIE: `SealMesh` on both spawned pawns had correct `SM_Seal`/
  `bOwnerNoSee=true` config). The actual bug is in `JinzzaCustomization::ApplyToMesh`
  (`jinzzaCustomizationApply.cpp`), called by `UjinzzaCharacterCustomizationComponent` on every
  pawn's `BeginPlay`: it creates a hair-color sphere (and an accessory cube) socketed to
  `CharacterMesh0`'s "Head" bone — but `CharacterMesh0` is the OLD Mannequin skeleton, now
  permanently `bHiddenInGame`/`bOwnerNoSee` since the real body became the separate static
  `SealMesh`. The hair sphere was never given matching hidden/owner-no-see flags, so **every
  player saw a small stray colored orb floating near their own character** instead of/alongside
  the seal — confirmed live (`CustomizationHairMesh`: `bHiddenInGame=false, bOwnerNoSee=false`).
  Fixed by syncing the hair/accessory components' hidden state to `Mesh`'s every time
  `ApplyToMesh` runs.
- [x] **Emote wheel moved from E to Tab**: `IA_EmoteWheel` was authored on E in `IMC_Default`,
  colliding with `IA_Interact` (also E — kiosks/props), so holding E to interact could also pop
  the emote wheel. Since this Input Mapping Context's `Mappings` array isn't reliably readable or
  editable through the editor-automation tooling available to these sessions (confirmed again this
  round), fixed it in C++ instead: `AjinzzaPlayerController::BuildRuntimeMappingContext` now
  force-remaps `IA_EmoteWheel` to Tab on the per-player runtime duplicate it already builds (same
  place per-player key rebinds are applied), leaving the shared `.uasset` and every other mapping
  untouched.
- [x] **`WBP_PropUsageHUD` dead `FClassFinder`**: same "not a child class" orphaned-Blueprint
  problem as `WBP_RoomSettings` last round (confirmed via log). Removed the lookup - but this one
  needed care: unlike the two kiosk classes, `AjinzzaCharacter::BeginPlay()` had **no**
  `StaticClass()` fallback when `PropUsageWidgetClass` is unset, so deleting the lookup outright
  would have silently broken the prop-usage HUD entirely. Defaulted `PropUsageWidgetClass` to
  `UjinzzaPropUsageWidget::StaticClass()` directly instead.

**Action needed**: compile `jinzzaCustomizationApply.cpp`, `jinzzaPlayerController.cpp`,
`jinzzaCharacter.cpp`. Then retest: in the Lobby, confirm no floating sphere near your character,
and confirm E only interacts (no emote wheel) while Tab opens the emote wheel.

---

## 1. Fixed previous round (2026-09-15) — needs a compile before it's live

- [x] **Phase-transition zone teleport was silently dead** — `AjinzzaGameGameMode::UpdateZoneForPhase`
  (committed 09-14) had never been compiled into the running editor (confirmed via
  `search_subclasses` + DLL/source timestamps). **You already fixed this by compiling** — verified
  afterward: `AjinzzaAuditionCurtain` now exists live, and all zone `PlayerStart` tags check out
  correctly in `Lvl_Game`.
- [x] **`jinzzaCharacterPreviewCapture.cpp`** referenced a skeletal mesh (`SKM_Seal`) that was
  deleted back on 09-14 when the real character switched to a static `SM_Seal` mesh — logged a CDO
  Constructor error on every compile, and the Customization-screen/main-menu preview was broken.
  Reverted to the last-known-good `SKM_Manny_Simple` + `MM_Idle` fallback. **Note**: the preview
  now shows the Mannequin, not the real seal body — see §3 below, this needs a real decision.
- [x] **`jinzzaRoomSettingsKiosk.cpp`** — removed a `ConstructorHelpers::FClassFinder` lookup for
  `WBP_RoomSettings` that could never succeed (confirmed via the live log: that Blueprint's
  generated class "is not a child class of jinzzaRoomSettingsWidget" — a stale leftover from before
  the 09-07 C++-widget-tree migration). Was silently falling back to the raw C++ class anyway; now
  it does so without an error every compile.
- [x] **`jinzzaFriendInviteKiosk.cpp`** — same cleanup for `WBP_FriendInvite`, which doesn't exist
  as an asset at all (confirmed via `find_assets`).
- [x] **`UjinzzaInteractionPromptWidget`** (the "[E] Interact" floating prompt over kiosks/props)
  had no Designer-authored tree and no C++ fallback — it was rendering **nothing**, ever. Gave it a
  `BuildWidgetTree()` (note-panel + centered text), matching the pattern already proven on 5 other
  widgets. Now shows real text.
- [x] **`UjinzzaPropUsageWidget`** (bottom-right "how to use this prop" HUD) had the same problem —
  nothing rendered when holding Megaphone/Bat/Boombox/etc. Gave it a `BuildWidgetTree()` too
  (icon + text row, anchored bottom-right).

**Action needed from you**: trigger a Live Coding compile (Ctrl+Alt+F11) or rebuild `jinzzaEditor`
for these 6 files to take effect: `jinzzaCharacterPreviewCapture.h/.cpp`, `jinzzaRoomSettingsKiosk.cpp`,
`jinzzaFriendInviteKiosk.cpp`, `UI/jinzzaInteractionPromptWidget.h/.cpp`, `UI/jinzzaPropUsageWidget.h/.cpp`.
Then a quick PIE pass: walk up to a kiosk (prompt should show real text) and pick up a noise prop
(usage HUD should appear bottom-right).

---

## 2. Verify next (should work now, not independently confirmed end-to-end)

- [ ] **Full phase-transition playtest**: with the zone system now actually compiled, run a
  multi-client PIE (or a real match) through at least one full phase cycle and confirm players
  visibly teleport zone-to-zone, the Self-Intro curtain rises/falls, and the Interview pair gets
  seated/immobilized and released correctly. This was verified by *code+data* inspection, not by
  watching it happen.
- [ ] **`AjinzzaAuditionCurtain` placement** — confirm it's actually placed in `Lvl_Game`'s
  Self-Intro zone (the 09-14 commit added the class but flagged it as "not yet placed, needs a
  compile first" — the compile has now happened; the placement step may still be outstanding).

---

## 3. Needs a decision from you (not a bug I can just fix)

- [ ] **Seal customization preview mismatch**: `UjinzzaCharacterPreviewCapture` and the whole
  `JinzzaCustomization::ApplyToMesh` pipeline (Head/HairColor/Accessory, socket-based) are built
  around a `USkeletalMeshComponent`. The real character's seal body is now a rigid
  `UStaticMeshComponent` (`SM_Seal`, no skeleton, no sockets — see `SealMesh` on
  `BP_FirstPersonCharacter`), because seal animation isn't built yet. As of this round the visible
  symptom (a stray floating hair/accessory orb - see §0) is fixed by hiding those components
  whenever the underlying skeletal mesh is hidden, but the underlying mismatch is still there: the
  Customization screen's live preview (and the main-menu background character) show the Mannequin,
  not the seal, and hair/accessory customization is now correctly invisible everywhere rather than
  ever actually showing on the seal. Options, roughly in order of effort: (a) leave it as-is until
  real seal animation exists (a proper skinned `SKM_Seal` + AnimBP), since the whole customization
  system will need rework then anyway; (b) attach hair/accessory as child *static* meshes to a
  fixed bone-free socket point on the capsule instead of a skeleton socket, sacrificing per-pose
  attachment; (c) drop face/hair/accessory customization for the seal entirely until animation
  lands. Tell me which and I'll implement it.
- [ ] **`WBP_RoomSettings` / `WBP_Settings` orphaned Blueprint assets**: now provably dead (see §1)
  — safe to delete from Content, or worth reparenting/re-authoring in the Designer if you want a
  real visual pass on those screens later instead of the current C++-built chrome. Your call; I
  didn't delete content without asking.

---

## 4. Quick manual checks only you can do (tooling can't read/click these)

- [ ] **`IMC_Sprint`'s mapped key** — still unreadable via the automation tooling (its `Mappings`
  struct array reads back empty even though the content is real, confirmed again this session).
  Open `IMC_Sprint` in the editor once and confirm/set the physical key (almost certainly Left
  Shift). Outstanding since 2026-09-07.
- [ ] **End-to-end UI click-through**: Host Game → Lobby → kiosk interact → Settings/Room
  Settings/Friend Invite panels. No input-injection tool exists, so button clicks have never been
  tested by a session, only "constructs without crashing."
- [ ] **Sprint key actually sprinting** — last user report was "the key does nothing"; the wiring
  reads correct in code, prime suspect is the IMC_Sprint key binding above. Retest after confirming
  the key.

---

## 5. Content/art still needed (blocks features that are otherwise code-complete)

- [ ] **`UjinzzaVoiceTestWidget`** and **`UjinzzaCustomizationWidget`** — both still fully
  Designer-`BindWidgetOptional`-dependent with **no** authored tree and **no** C++ fallback (unlike
  the 7 widgets already fixed across this project's history). Both are large (10+ and 25+ bound
  widgets respectively) — too big to safely improvise without a real layout pass. These currently
  show **nothing** when opened. Worth a dedicated session once you're ready to invest in their
  layout, or tell me to build a plain functional (unstyled-but-working) C++ tree for them the same
  way as the smaller widgets in §1.
- [ ] **Question and Evaluation zones** in `Lvl_Game` are still bare gray-box floors (per README's
  own roadmap table) — Self-Intro (stage+spotlight+curtain), Free Time (real noise props), and
  Interview (desk/chairs/isolation walls/spotlight) are dressed; these two aren't.
- [ ] **Emote wheel / interaction-prompt icon art**: `F_Prompt` key-icon and the 4 emote icons are
  referenced in comments as "borrow from the noob-game reference project" but that project's actual
  image assets were deliberately never imported (style mismatch, per 09-02's session). Needs either
  real icon art or a decision to keep the current text-only fallback.

---

## 6. Bigger systems not started yet (design doc weeks 7, 8, 12+)

- [ ] **Week 7 — Question time & voting**: `UQuestionTimeSubsystem` (simultaneous submit + reveal),
  `AVoteManager`, `WBP_VoteUI`. Nothing exists yet beyond the phase-timing placeholder.
- [ ] **Week 8 — 1:1 interview & ghost state**: real judge-picks-a-target system (today's
  `EnterInterviewZone` auto-pairs Judge + first candidate as a placeholder), isolated interview
  voice channel, ghost-state transition on mid-evaluation elimination (`EliminateToGhost` exists on
  `AjinzzaGameGameMode` but nothing calls it yet).
- [ ] **2-judge mode** and **"Host Picks" role assignment**: both explicitly deferred fallbacks
  (`AssignRoles` always does 1 judge / random) per the design doc's own "undecided" flag — pick up
  only once those rules are actually settled in the design doc.
- [ ] **Lobby-customization snapshot cloning**: `AssignRoles()` only clones `FaceType`/
  `VoiceFilter` onto Imitators, not full appearance — blocked on `UCharacterCustomizationComponent`
  design decisions in §3 above.
- [ ] **Auto mid-evaluation-count scaling by player count** (Week 11) — `VoteCount` is a fixed lobby
  setting today, not derived from player count.
- [ ] **Week 12+ — sound/art pass, playtesting balance, Steam store page, QA.**

---

## 7. Accounts / external setup (only you can do these)

- [ ] **Steamworks partner account + real App ID** — replace the placeholder
  `SteamDevAppId=480` in `Config/DefaultEngine.ini` once issued.
- [ ] **Epic Games Dev Portal org → Product → Client**, EOS credentials into
  `DefaultEngine.ini` — unblocks Week 6 voice (`EOS Voice Chat`, already bundled + API-verified,
  see `docs/PROJECT_STATUS.md` §8 for the technical detail). This is the single blocker on the
  entire voice/disguise-voice-filter system.

---

## 8. Housekeeping

- [ ] Everything above (and everything since the last commit) is **uncommitted** — same standing
  note as every prior session. Ask when you want a commit made.
- [ ] `todo.txt` is now 2700+ lines — consider archiving everything before, say, 09-01 into a
  `todo_archive.txt` if it gets unwieldy to search. Not urgent.
