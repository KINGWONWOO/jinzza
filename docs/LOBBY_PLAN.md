# Lobby (`Lvl_Lobby`) — meshes needed & what's left to do

_Written 2026-10-04 after a pass that re-lit, re-laid-out and optimized `Lvl_Lobby` (details at the end)._

## 1. Meshes the lobby still needs

Conventions so a new mesh drops straight in:
- **Units:** cm.
- **Pivot:** bottom-centre.
- **Front:** for kiosks, the side players use faces the actor's **−X** (that's where the kiosk labels face, and how the kiosks are placed now).
- **Format:** the existing Tripo/Blender pipeline is fine. Bake the scale in Blender; there's a UE Y-mirror gotcha. The kiosk swaps the mesh, so no code change is needed as long as pivot and front follow these rules.

### A. Must-have — currently engine primitives (they look like placeholders in-game)

| # | Asset (suggested name) | Used by | Now | What it should be | Rough size (W×D×H) |
|---|---|---|---|---|---|
| 1 | `SM_Kiosk_StartMatch` | `AjinzzaStartMatchKiosk` (host starts the match; in front of the fireplace) | grey engine cylinder | a "go" object: big bell / lever / button on a stand | 100×100×120 |
| 2 | `SM_Kiosk_Wardrobe` | `AjinzzaWardrobeKiosk` (opens Customize; bedroom corner) | grey engine cylinder | wardrobe or mirror + clothes rack | 150×60×200 |
| 3 | `SM_LobbyClock_Body` + `SM_LobbyClock_HourHand` + `SM_LobbyClock_MinuteHand` | `AjinzzaLobbyClock` (press E → Day/Sunset/Night; wall above the Invite desk) | engine cylinder face + cube hands, dark and hard to see | wall clock. **Hands must be separate meshes** with their pivot at the rotation centre (the code rotates them) | face ~60–100 Ø |

### B. Nice-to-have — working stand-ins borrowed from the Noob project

| # | Asset | Used by | Now | Upgrade idea |
|---|---|---|---|---|
| 4 | `SM_Kiosk_RoomSettings` | `AjinzzaRoomSettingsKiosk` | Noob `SM_Board` (small chalkboard; per-instance override in the level) | notice board / big chalkboard on an easel, ~150×60×200 |
| 5 | `SM_Kiosk_FriendInvite` | `AjinzzaFriendInviteKiosk` | Noob `SM_Desk` (small desk, per-instance override) | reception desk with a phone / letters, ~150×70×100 |
| 6 | Ready-area marker | `Label_ReadyArea` text at the rug centre | floating English text | floor decal or circular mat under the 12 spawn points |

### C. Room shell (biggest visual upgrade)

The room's floor, walls, ceiling, roof and the outdoor ground are **51 stretched copies of the engine template `SM_Template_Map_Floor`** (a 96-triangle box with a material override). They work, but there's no window framing, skirting or real thickness.

A small **modular cabin kit** would replace them: floor tile, plain wall, wall with window, wall with door frame, ceiling beam/plank, roof panel, corner trim. Keep a 100 cm grid. The room is about 5000×4800 cm, ceiling around 2200.

### D. Decided by the design doc, not yet chosen

- **Customization parts** (colour / outfit / accessory) for the lobby wardrobe. The design doc (§12–13 and the open questions) says the SD low-poly asset pack isn't chosen yet, so this list waits on that decision.

Everything else in the room (sofa, bed, table, chairs, crates, fireplace, lamps, rug, shelves, curtains, the 43 Stylized_Forest trees outside) is fine as is.

## 2. What you need to do

1. **Rebuild with the editor closed.** Several C++ changes from today, including a new Build.cs module, need a full build, not Live Coding.
2. **PIE the lobby** with Day / Sunset / Night (press E at the clock). Check:
   - you spawn on the rug ring, not inside furniture;
   - each kiosk is reachable and its prompt appears;
   - the sunbeam lands on the rug at Day;
   - Night is dark enough for your taste. If not, lower the post-process volume's max exposure (`AutoExposureMinBrightness`, currently −7).
3. **Make or source meshes A1–A3** (and B4–B6 / C if you want), following the conventions above. To swap one in, set the kiosk's `Mesh` → Static Mesh, either on the level instance (what RoomSettings / FriendInvite do now) or in the kiosk's C++ constructor default.
4. **Decide:**
   - **Voice Test kiosk:** it exists (`AjinzzaVoiceTestKiosk`, uses `SM_StandMic`) but is only placed in `Lvl_test`. Put one in the lobby?
   - **Duplicate English labels:** the floating TextRender labels (`START MATCH (Host Only)`, `READY AREA`, `LOBBY`) duplicate the kiosks' Korean labels. Keep or delete?
   - **Customization asset pack:** see D.
5. **Commit:** `Lvl_Lobby.umap` plus the three Megascans meshes switched to Nanite (`S_Firewood_..._Var1`, `S_Wooden_Floor_Lamp_..._Var1/_Var2`).

## 3. What was changed in this pass

**Optimization**
- Deleted the duplicate fireplace: `StaticMeshActor_179`, a second copy sitting almost exactly on top of the first.
- Turned on **Nanite** for the Megascans firewood (36 copies × 12k triangles ≈ 432k) and the two floor-lamp meshes (~10k each). None of them were Nanite before.

**Layout**
All kiosks were floating 1–1.2 m above the floor and bunched within 4 m of the centre, on top of each other and the "READY AREA" text. Now:

| Actor | Position | Facing |
|---|---|---|
| Start Match | in front of the fireplace (1300, 0) | the room |
| Room Settings board | by the sofa (1050, −800) | the rug |
| Invite Friends desk | by the entrance door (−1950, 900) | — |
| Wardrobe | bedroom corner (−950, −1900) | — |
| `LobbyClock` (newly placed, 2.5× scale) | west wall above the Invite desk | — |

- All kiosks are snapped to the floor (floor top z = −39).
- **12 player starts:** were on a 20 m-radius ring where slot 1 was inside the fireplace and slot 5 overlapped a dining chair. Now a 7 m ring on the centre rug, facing inward, at z 60.
- **Labels:**
  - `LOBBY` title: was outside the south wall, now above the door;
  - `START MATCH (Host Only)`: now above the Start kiosk;
  - `READY AREA`: now at the rug centre.

**Sun & sky**
- **Sun:** was at a −5° grazing angle with a 112° roll, which didn't match the runtime Day preset (−50° / 20°). With the runtime angle almost no light entered the room.
  - Now pitch −25 / yaw −100. It comes in through the north window wall and lands a sunbeam on the rug.
  - The C++ presets in `jinzzaLobbyGameState.cpp` were updated to match: Day = level; Sunset (−10, −115) warm; Night (−40, −120) dim and cool.
- **Sky:** added a **SkyAtmosphere**. The sun was flagged as the atmosphere sun, but there was no sky.
- **SkyLight:** Stationary → **Movable + real-time capture** (Lumen-friendly), intensity 3.
- **Interior lights (new, warm, movable, shadowed):** the room had none.
  - `Light_FloorLamp_East` and `Light_FloorLamp_West`: 40 cd, 2700 K
  - `Light_Fireplace`: 60 cd, 1900 K
  - `Light_Bedroom`: 25 cd, 2700 K
  These aren't touched by the time-of-day presets, so they carry the room at Night.

**Post-process volume** (unbound)
- **Exposure:** was Basic auto-exposure with a +2.7 bias and no range, which caused "Lumen cached lighting clipped" warnings and blown highlights.
  - Now Histogram, bias 0, range EV100 −7…−4.
  - It was tested in steps: −4 / −3 / 2 / 5 were too dark; the room is physically dim at 10 lux.
- **Grading:** saturation 1.2 → 1.1, contrast 1.12 → 1.05, bloom 0.9 → 0.5. The warm shadow/highlight colour gains are kept.

**Not verified:** looked at only in editor viewport captures, not in PIE or a packaged build. The new lights and the sky were not checked for performance on low-end PCs.
