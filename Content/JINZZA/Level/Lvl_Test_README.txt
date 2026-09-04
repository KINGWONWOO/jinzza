Lvl_test - Feature Test Level
==============================

Purpose: a running "display case" for every testable gameplay feature in the project.
Walk forward from the PlayerStart through the numbered zones; each one is self-contained
and has an in-world sign explaining what to try and which key does what.

Zones (in order, spaced ~460 units apart along +X):

  1. Bat            - F to pick up, Left Click to swing (knocks back whoever's in front),
                       Q to drop, Right Click to throw.
  2. Boombox         - Placed prop; F toggles the looping track on/off in place.
  3. Megaphone       - F to pick up, Left Click toggles voice amplification (no-op until
                       the proximity voice system exists - see PROJECT_STATUS.md).
  4. Basketball+Hoop - F to pick up the ball, Left Click to dribble/bounce, Right Click to
                       throw. The hoop next to it detects a score when the ball passes
                       through its rim trigger.
  5. Emote Wheel     - No prop; this tests AjinzzaCharacter itself. Hold E to open the
                       radial menu, move the mouse into a quadrant, release E to play that
                       emote (ThumbsUp/ThumbsDown/MiddleFinger/Point).
  6. Steal & Throw   - A spare Boombox to test throwing distance solo, and a reminder that
                       stealing (pressing F on a prop someone else is already holding) needs
                       a second player/client to actually test.
  7. Stun Gun        - F to pick up, Left Click zaps whoever's in front (a real, working
                       immobilize - AjinzzaCharacter::Stun blocks their movement/jump input
                       for ~3s), Right Click to throw. The "victim's voice sounds mechanical"
                       half of this feature is NOT implemented - see BP_StunGun/
                       AjinzzaStunGunProp's class comment: it needs the proximity voice
                       system (Vivox/EOS, Week 6), which doesn't exist yet.

Also present everywhere in this level (not zone-specific):
  - Interaction prompts: look at any prop and a small on-screen prompt appears above it
    ("Pick Up" / "Use"). The prompt's F-key icon is still unassigned - see
    WBP_InteractionPrompt (needs the noob-game project's F_Prompt icon, which is currently
    only a Git LFS pointer in this checkout; fetch it there and drop it into the Image
    widget once available).
  - Prop usage HUD: pick anything up and a bottom-right HUD panel explains how to use it
    (see WBP_PropUsageHUD). It disappears when you drop, throw, or lose the prop.

Note on placeholder art: every prop's mesh here is an Engine basic shape (cylinder, cube,
cone, sphere) standing in for real art - see each Blueprint's Mesh component. Swap these
out once real meshes exist; nothing else needs to change.

Convention going forward: whenever a new testable feature is added to the project, add a
new numbered zone here for it (prop/actor + a TextRenderActor sign, same pattern as above)
rather than leaving it untested outside of code review.
