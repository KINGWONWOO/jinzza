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

Note on Korean text and fonts (RESOLVED 2026-09-09): the CAVEAT below was confirmed true -
a viewport capture showed the 16 standalone signs rendering as blank "Tt" placeholder
icons, not Korean glyphs, because TextRenderComponent cannot render a Runtime-cached
composite UFont (only "Offline Cached" bitmap UFonts work, and baking one needs Unreal's
native font-import step, which has no unreal-mcp equivalent). Fix: every piece of Korean
text in this level now goes through UMG/Slate instead of TextRenderComponent:
  - The 16 standalone signs are BP_WorldSign actors (World Space WidgetComponent, see
    "Exhibit booths" above).
  - The 5 kiosk labels (Wardrobe/RoomSettings/FriendInvite/StartMatch/VoiceTest) each got
    a SECOND WidgetComponent ("KoreanLabelWidget") added directly onto the existing C++
    kiosk actor instance in the level (no C++ change or recompile needed - see
    [[unreal-mcp-gotchas]] gotcha #11), parented to Mesh at the same relative transform the
    old Label had. The original Label TextRenderComponent is still there but hidden
    (bVisible=false) since it can't be removed from a C++-constructed instance.
All 21 signs are separate small widget classes (/Game/JINZZA/UI/Widgets/Signs/WBP_Sign_XX_*)
each with their own Korean text baked into the class default, rather than one shared class -
see [[unreal-mcp-gotchas]] gotcha #10 for why.

Note on lighting (fixed 2026-09-08): this level had no DirectionalLight or SkyLight at all
until now - it would have PIE'd essentially pitch black. Added both, matching Lvl_Lobby's
DirectionalLight rotation (Pitch -50, Yaw 20).

Exhibit booths (2026-09-09, fifth and current revision - see below for earlier ones):
the U-shaped booth walls (back wall + two side arms per zone) were removed. Each zone's
prop/kiosk now sits in the open at Y=280 with a single non-functional door-frame prop
(BP_DoorFrame_Decor, in the "ZoneDoorBackdrops" outliner folder) standing behind it at
Y=460, X = zone centerline - 30 (matching the scale/pose of the original hand-placed
"Zone14_Door2" reference instance the user built, scale 0.75, yaw=0). BP_DoorFrame_Decor
is a duplicate of BP_DoorFrame (LevelPrototyping Interactable/Door kit) with its entire
EventGraph (BeginPlay/overlap detection/Open/CloseDoor timeline) deleted - it keeps only
the UserConstructionScript "Set Mesh" logic that assembles the door frame's visual mesh,
so it looks identical to a working door but has no interact/open logic or collision
response. Zones are still spaced 700 units apart along X, from X=0 (zone 1) through
X=9800 (zone 15); every prop/kiosk/sign is still rotated to face -Y as before.

This is the fifth pass at this design: dividers between zones (blocked the walkway) ->
alternating-side booths with actors on the old Y=0 centerline -> uniform-direction booths
with actors centered inside them -> fourth pass added zones 10-15, section header signs,
Korean translation, and composite prop meshes -> this version, which replaces the
U-shaped booth walls with a decorative door standing behind each zone's actor, and fixes
the Korean sign text (see below).

Korean text fix (2026-09-09): the U-shaped wall removal correlated with also fixing the
long-standing Korean rendering bug described below. All 16 standalone signs (welcome +
5 section headers + zones 1-8, 14, 15) were converted from TextRenderActor to a new
BP_WorldSign actor (Content/JINZZA/Level/BP_WorldSign) - a World Space WidgetComponent
hosting a small UMG widget (Content/JINZZA/UI/Widgets/WBP_WorldSign, one TextBlock bound
to a SignText variable, Sacheon Uju font). Since unreal-mcp has no tool to author a
Widget Blueprint's visual tree, each sign is a separate duplicate of WBP_WorldSign
(Content/JINZZA/UI/Widgets/Signs/WBP_Sign_XX_Name) with its own SignText baked into the
class default, rather than one shared class driven by a per-instance variable (a
cross-blueprint variable/function-call node could not be created via the available
graph-editing tools against a not-yet-open Blueprint class - if that limitation is ever
lifted, this could be collapsed back to one shared widget class). NOTE: the 5 kiosk
Label sub-components (Wardrobe/RoomSettings/FriendInvite/StartMatch/VoiceTest) are still
plain C++ TextRenderComponents and were NOT converted - they still won't render Korean
text until either re-imported as an Offline bitmap font or their C++ Label component is
swapped for a WidgetComponent (deferred - see [[deferred-features]]).

Convention going forward: whenever a new testable feature is added to the project, add a
new numbered zone here for it (prop/actor + a TextRenderActor sign in Korean, same pattern
as above), assign it to one of the 5 sections (or add a new section), and keep this file in
sync - rather than leaving it untested outside of code review.
