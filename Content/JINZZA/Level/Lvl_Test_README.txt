Lvl_test - Feature Test Level
==============================

Purpose: a running "display case" for every testable gameplay feature in the project.
Walk forward from the PlayerStart through the numbered zones; each one is self-contained
and has an in-world sign explaining what to try and which key does what. All in-level
signage is now in Korean (2026-09-08).

The level is divided into 5 sections, each with its own header sign on the corridor
centerline (Y=0, facing back toward -X like the welcome sign):

  Section A - 소음 소품 (Noisy Props)     : zones 1-3
  Section B - 이동 (Movement)              : zones 4, 6, 7, 8 (basketball/steal/stungun/jumppad)
  Section C - 감정표현 (Expression)        : zone 5 (Emote Wheel)
  Section D - 키오스크 & 로비 기능 (Kiosks): zones 9-13
  Section E - 레벨 프로토타이핑 (Prototyping): zones 14-15

Zones (in X order along the corridor; each prop/kiosk and its sign sit at Y=280,
facing -90 yaw toward the main corridor at Y=0 - see "Exhibit booths" below):

  1. 배트 (Bat)              - F to pick up, Left Click to swing (knocks back whoever's in
                       front), Q to drop, Right Click to throw.
  2. 붐박스 (Boombox)        - Placed prop; F toggles the looping track on/off in place.
  3. 메가폰 (Megaphone)      - F to pick up, Left Click toggles voice amplification (no-op
                       until the proximity voice system exists - see PROJECT_STATUS.md).
  4. 농구공 + 골대 (Basketball+Hoop) - F to pick up the ball, Left Click to dribble/bounce,
                       Right Click to throw. The hoop detects a score when the ball passes
                       through its rim trigger.
  5. 이모트 휠 (Emote Wheel) - No prop; this tests AjinzzaCharacter itself. Hold E to open
                       the radial menu, move the mouse into a quadrant, release E to play
                       that emote (ThumbsUp/ThumbsDown/MiddleFinger/Point).
  6. 훔치기 & 던지기 테스트 (Steal & Throw) - A spare Boombox to test throwing distance
                       solo, and a reminder that stealing (pressing F on a prop someone
                       else is already holding) needs a second player/client to test.
  7. 스턴건 (Stun Gun)       - F to pick up, Left Click zaps whoever's in front (a real,
                       working immobilize - AjinzzaCharacter::Stun blocks their
                       movement/jump input for ~3s), Right Click to throw. The "victim's
                       voice sounds mechanical" half of this feature is NOT implemented -
                       needs the proximity voice system (EOS Voice Chat, Week 6).
  8. 점프대 (Jump Pad)       - Walk onto the pad to launch into the air (Space also still
                       jumps normally everywhere). The walk down the hallway doubles as the
                       Sprint (hold Left Shift) test.
  9. 옷장 (Wardrobe)         - AjinzzaWardrobeKiosk. F to open the same
                       character-customization panel as the main menu's Customize button -
                       cycles through the placeholder Head/HairColor/Top/Eyebrows/Eyes
                       options live.
 10. 방 설정 (Room Settings) - AjinzzaRoomSettingsKiosk. F to open the room settings panel.
 11. 친구 초대 (Friend Invite) - AjinzzaFriendInviteKiosk. F to open the friend invite panel.
 12. 매치 시작 (Start Match) - AjinzzaStartMatchKiosk. F to open the start-match panel.
                       CAUTION: using this can leave the test level (starts a real match
                       flow) - sign notes this in-world.
 13. 음성 테스트 (Voice Test) - AjinzzaVoiceTestKiosk. F to open a live mic-loopback panel:
                       Pitch/Robot/Cave Echo sliders you can drag directly, plus
                       Cave/Helium/Robot template buttons. Local-only playback (speak into
                       your mic, hear yourself filtered) - not networked, and separate from
                       the round-disguise voice filter and the still-blocked proximity voice
                       system (EOS Voice Chat, Week 6).
 14. 문 (프로토타이핑) (Door) - BP_DoorFrame from the LevelPrototyping kit's Interactable
                       set. A generic interactable-door example (no custom sign self-label,
                       so a TextRenderActor sign was added next to it).
 15. 흔들리는 표적 (프로토타이핑) (Wobble Target) - BP_WobbleTarget from the LevelPrototyping
                       kit's Interactable set. A generic throw-accuracy target example
                       (same as above, sign added alongside it).

Also present everywhere in this level (not zone-specific):
  - Interaction prompts: look at any prop and a small on-screen prompt appears above it
    ("Pick Up" / "Use"). The prompt's F-key icon is still unassigned - see
    WBP_InteractionPrompt (needs the noob-game project's F_Prompt icon, which is currently
    only a Git LFS pointer in this checkout; fetch it there and drop it into the Image
    widget once available).
  - Prop usage HUD: pick anything up and a bottom-right HUD panel explains how to use it
    (see WBP_PropUsageHUD). It disappears when you drop, throw, or lose the prop.

Note on placeholder art (updated 2026-09-08): every prop's mesh here is now a small
COMPOSITE of Engine basic shapes (cylinder/cube/cone/sphere) approximating the real
silhouette, rather than a single primitive - e.g. BP_Bat has a thin shaft plus a thicker
"Barrel" cylinder near the top; BP_Boombox has its body cube plus two speaker cylinders and
a handle bar; BP_Megaphone has its cone horn plus a handle cylinder; BP_BasketballHoop has
its rim disc plus a backboard cube and a pole cylinder down to the ground; BP_StunGun has
its body cylinder plus a barrel cylinder and a grip cube. Swap these out for real meshes
once they exist; nothing else needs to change. (Basketball itself was left as a plain
sphere - already an accurate shape.)

Note on Korean text and fonts (2026-09-08): all TextRenderComponents in this level
(zone/section/welcome signs) and all kiosk Label components now have their Font property
set to /Game/JINZZA/Fonts/SacheonUju-Regular_Font, the project's only Korean font asset.
CAVEAT: this UFont asset has FontCacheType = Runtime, which is the Slate/UMG font
pipeline; TextRenderComponent historically expected an "Offline Cached" bitmap UFont. This
session could not visually verify in a running editor/PIE session whether Runtime-cached
fonts render correctly on 3D world-space TextRenderComponents in UE 5.8 - if the Korean
text on these signs appears blank/missing glyphs/tofu boxes when you actually look at the
level, that's the likely cause. (The UMG/Slate UI text pipeline elsewhere in the project
already uses this same font successfully - this risk is specific to world-space
TextRenderActor/TextRenderComponent signs, not menus/HUDs.)

Note on lighting (fixed 2026-09-08): this level had no DirectionalLight or SkyLight at all
until now - it would have PIE'd essentially pitch black. Added both, matching Lvl_Lobby's
DirectionalLight rotation (Pitch -50, Yaw 20).

Exhibit booths (2026-09-08, fourth and current revision - see below for earlier ones):
every zone's prop/kiosk and its sign sit together at Y=280, the CENTER of a U-shaped booth
(a back wall + two side arms, open toward the main corridor at Y=0). All 15 booths open
the same direction (toward -Y) - the corridor at Y=0 and below stays a clear,
uninterrupted walkway the full length of the level, and every zone's booth is a short walk
to the +Y side of it. Every prop/kiosk/sign is also rotated to yaw=-90 so it visually
faces back out through the booth's open mouth (kiosks like Wardrobe/RoomSettings/
FriendInvite/StartMatch/VoiceTest are yaw=+90 instead, since their own Label sub-component
has a fixed +180 relative rotation baked into the C++ constructor - net effect is the same
-Y-facing label).

Booths are built from SM_Cube segments (LevelPrototyping kit, corner-pivoted - back wall
is 2 segments, each side arm is 1), 30 units thick, 220 tall, with the two side arms 240
units either side of the zone's centerline (480 apart total) and 400 deep. All booth
pieces carry a shared MI_ExhibitBoothWall material instance (Content/JINZZA/Level/
MI_ExhibitBoothWall, a flat-color instance of the LevelPrototyping kit's M_FlatCol - light
neutral gray, no grid texture). Zones are spaced 700 units apart along X, from X=0 (zone 1)
through X=9800 (zone 15). The floor spans X: -700..10200, Y: -500..600. All booth pieces
live in the "ExhibitBooths" outliner folder; the material instance is a plain content
asset, not inside the level.

This is the fourth pass at this design: dividers between zones (blocked the walkway) ->
alternating-side booths with actors on the old Y=0 centerline -> uniform-direction booths
with actors centered inside them -> this version, which adds zones 10-15 (RoomSettings/
FriendInvite/StartMatch/VoiceTest kiosks + Door/WobbleTarget prototyping examples), section
header signs, Korean translation of every sign, and composite prop meshes.

Convention going forward: whenever a new testable feature is added to the project, add a
new numbered zone here for it (prop/actor + a TextRenderActor sign in Korean, same pattern
as above), assign it to one of the 5 sections (or add a new section), and keep this file in
sync - rather than leaving it untested outside of code review.
