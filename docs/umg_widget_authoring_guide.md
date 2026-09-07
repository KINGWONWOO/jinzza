# UMG Widget Authoring Guide

**Status (2026-09-07): superseded for now.** The PIE-blocking compile errors this guide was
written for (`LogBlueprint: Error: ... A required widget binding "X" ... was not found` on
WBP_MainMenu/WBP_Settings/WBP_Lobby/WBP_RoomSettings/WBP_GameEnd) are fixed by a different route:
each of the five `UjinzzaXWidget` C++ classes now has a `BuildWidgetTree()` that constructs its
own UMG tree at runtime (via `WidgetTree->ConstructWidget` + the `JinzzaUI::Make*` style
helpers), and `meta = (BindWidget)` was removed from their properties, so the WBP Designer trees
no longer need to exist at all for PIE to run. This was a deliberate choice to unblock testing
without doing the visual design pass yet — not the "no MCP tool can author Designer trees"
limitation being un-blocked. That limitation is still real (see below).

**When this guide becomes relevant again**: once you move into the actual UI design/art pass.
At that point, for each widget: delete its `BuildWidgetTree()` function, restore
`meta = (BindWidget)`/`BindWidgetOptional` on its properties (each header still has a comment
marking exactly this), and build the real layout in the WBP's Designer using this guide. Every
widget/name/type pairing below is still accurate — `BuildWidgetTree()` was written to construct
the same names and roughly the same layout described here, just as C++ instead of a Designer
tree, so this doc doubles as a description of what the current (temporary) C++ layout looks like
too.

**Why a Designer tree can't be authored via tooling instead**: these Widget Blueprints were
created via unreal-mcp, but no MCP tool can place child widgets inside a WBP's visual tree —
that part has always been manual (see `Source/jinzza/UI/jinzzaMainMenuWidget.h`'s header
comment, and every other widget header in that folder). This doc is the reference for that: for
each WBP, the exact widgets to add, what to name them, and how to style them so they match the
rest of the game.

**Good news**: every click handler, combobox option list, and slider/spinbox min-max range is
already wired in C++ (`NativeOnInitialized` / `Populate*Page` in each `.cpp`). You do **not**
need to touch the Event Graph or set any "Is Variable" logic beyond checking the box — the only
jobs in the Designer are:
1. Add a widget of the right **type**, name it **exactly** as listed (case-sensitive), and check
   its **"Is Variable"** box (UMG only checks this automatically for BindWidget names it already
   sees registered from the parent class — if a name doesn't show that link, retype it exactly).
2. Nest it wherever makes visual sense (suggested layouts below).
3. Set its appearance (color/font/brush) to match the style values below — cosmetic only, no
   binding is required for these.

Do this for one WBP, compile it (Compile button in the WBP editor, or Ctrl+Alt+F11 for the whole
project), and check the Compiler Results panel at the bottom clears the "widget binding ... not
found" errors for that asset before moving to the next.

---

## 0. Shared style reference

Use these everywhere for visual consistency (from `Source/jinzza/UI/jinzzaUIStyle.cpp` — the
canonical values; nothing here is guessed).

**Colors** — these are **linear** RGBA. In UE's color picker, click the swatch, then in the
picker dialog uncheck the **"sRGB"** box at the bottom — the R/G/B/A fields then accept these
0–1 numbers directly with no conversion.

| Name | R | G | B | A | Used for |
|---|---|---|---|---|---|
| Color_Background | 0.035 | 0.030 | 0.045 | 1.0 | Full-screen backdrop |
| Color_Panel | 0.075 | 0.065 | 0.090 | 0.94 | Panel/border fill |
| Color_PanelBorder | 0.30 | 0.24 | 0.15 | 0.6 | Panel outline |
| Color_Accent (gold) | 0.85 | 0.68 | 0.25 | 1.0 | Primary CTAs, titles, active-tab accent, dividers |
| Color_AccentAlt (crimson) | 0.75 | 0.09 | 0.12 | 1.0 | Warning/destructive buttons (Quit, End Game) |
| Color_TextPrimary | 0.95 | 0.95 | 0.96 | 1.0 | Normal text |
| Color_TextMuted | 0.62 | 0.60 | 0.65 | 1.0 | Secondary/status text, secondary-button labels |
| Button Normal | 0.11 | 0.095 | 0.14 | 0.95 | Primary/Warning button base fill |
| Button Hovered (gold-tint) | 0.17 | 0.145 | 0.10 | 0.97 | Primary button hover |
| Button Pressed (gold-tint) | 0.22 | 0.17 | 0.08 | 1.0 | Primary button pressed |
| Button Hovered (secondary) | 0.14 | 0.14 | 0.16 | 0.85 | Secondary button hover |
| Button Pressed (secondary) | 0.18 | 0.18 | 0.20 | 0.95 | Secondary button pressed |
| Button Normal (secondary) | 0.09 | 0.09 | 0.10 | 0.7 | Secondary button base fill |
| Warn Hovered | 0.35 | 0.07 | 0.09 | 0.97 | Warning button hover |
| Warn Pressed | 0.45 | 0.05 | 0.08 | 1.0 | Warning button pressed |
| Button Disabled | 0.08 | 0.08 | 0.09 | 0.6 | Any button, disabled state |

**Font**: `/Game/JINZZA/Fonts/SacheonUju-Regular_Font` for everything. Sizes: Title 48
(main-menu logo), Heading 20–24 (section headings, button labels ~18–22), Body 15–16 (status
text, row labels, values).

**Button style** (apply to every `Button`'s Style properties in Details > Appearance):
- If `/Game/JINZZA/UI/Textures/T_ButtonPill` exists in Content Browser: set Normal/Hovered/
  Pressed images to it, Draw As = **Box**, Margin = **(0.19, 0.08, 0.19, 0.08)**, tint each
  state with the matching color above (Normal/Hovered/Pressed from the "Button Normal/Hovered/
  Pressed" rows — use the gold-tint pair for primary/warning buttons per their table row, the
  secondary pair for secondary buttons), Disabled tint = Button Disabled.
- If that texture isn't there yet (per [[fkey-icon-lfs-pointer]] some art assets are still LFS
  pointers — check first): fall back to a plain **Rounded Box Brush**, corner radius **6**,
  outline color = Color_Accent (primary/warning) or Color_TextMuted (secondary), outline width
  **1.5**.
- Padding: Normal (16, 10), Pressed (16, 11, 16, 9).
- Button label TextBlock color: Color_TextPrimary for primary/warning buttons, Color_TextMuted
  for secondary buttons.

**Panel background** (Border widgets used as panel/section containers): Brush = Rounded Box
Brush, fill = Color_Panel, corner radius **10**, outline color = Color_PanelBorder, outline
width **1.5**.

**Divider** (small accent bar under a title): a `SizeBox` sized **120×3**, containing a `Border`
with a Rounded Box Brush filled Color_Accent, corner radius 1.5 (Height/2), no padding.

---

## 1. WBP_MainMenu — parent `UjinzzaMainMenuWidget`

### Required (must exist or the WBP won't compile)
| Name | Type |
|---|---|
| Switcher | Widget Switcher |
| StatusText | Text (TextBlock) |
| HostButton | Button |
| SettingsButton | Button |
| QuitButton | Button |
| SettingsWidget | User Widget — **set its Class to `WBP_Settings`** in Details |
| ButtonsPageRoot | Any Widget (e.g. Vertical Box — this becomes Switcher's page 0 root) |

### Optional (`BindWidgetOptional` — skip for now, code already null-checks them)
`CustomizationButton`, `CustomizationWidget` (needs a `WBP_Customization` that doesn't exist
yet — leave out), `CharacterPreviewImage`, `VoiceTestWidget` (blocked on voice, see
[[eos-voice-chat-plan]] and [[deferred-features]]) — add these later once their backing content
exists.

### Suggested hierarchy
```
Canvas Panel (root, anchors 0,0–1,1, full screen)
└─ Border "Background"  (Brush = Color_Background, solid, no border)
   └─ Switcher                              ← BindWidget: Switcher
      ├─ [Page 0] Vertical Box "ButtonsPageRoot"   ← BindWidget: ButtonsPageRoot
      │    (place inside a Size Box ~420×520, centered via Canvas anchor 0.5/0.5,
      │     Vertical Box alignment = center, spacing between children ~16px)
      │    ├─ Text "Title"        "JINZZA" — Title font 48, Color_Accent, centered
      │    ├─ Divider (see §0)
      │    ├─ Text "StatusText"                    ← BindWidget: StatusText
      │    │     Body font 16, Color_TextMuted, centered, leave text empty (set at runtime)
      │    ├─ Button "HostButton" + child Text "Host Game"     ← BindWidget: HostButton
      │    │     Primary style, Heading font 22
      │    ├─ Button "SettingsButton" + child Text "Settings"  ← BindWidget: SettingsButton
      │    │     Secondary style, Heading font 18
      │    └─ Button "QuitButton" + child Text "Quit"          ← BindWidget: QuitButton
      │          Warning style, Heading font 18
      └─ [Page 1] User Widget "SettingsWidget", Class = WBP_Settings, fill screen
                                                    ← BindWidget: SettingsWidget
```
Switcher's **Active Widget Index** default = 0 (button page); code flips to 1 on Settings click.

---

## 2. WBP_Settings — parent `UjinzzaSettingsWidget`

Per `todo.txt`'s 2026-09-02 layout note, this follows the reference project's Option.png
arrangement: **left sidebar (fixed 160px) + right content column + bottom-right Apply/Back
pair**. All controls' behavior (options, ranges, click handlers) is already wired in C++ —
Designer only needs the widgets present and named.

### Required — chrome
| Name | Type |
|---|---|
| TabSwitcher | Widget Switcher |
| ApplyButton | Button |
| BackButton | Button |
| GraphicsTabButton / AudioTabButton / ControlsTabButton / GameplayTabButton | Button |
| GraphicsTabAccent / AudioTabAccent / ControlsTabAccent / GameplayTabAccent | Any Widget (a thin Border bar works) |

### Required — Graphics tab (TabSwitcher page 0)
| Name | Type |
|---|---|
| WindowModeCombo | ComboBox (String) |
| ResolutionCombo | ComboBox (String) |
| VSyncCheckBox | Check Box |
| FrameRateLimitSpinBox | Spin Box |
| OverallQualityCombo | ComboBox (String) |
| ViewDistanceSpinBox | Spin Box |
| ShadowSpinBox | Spin Box |
| GlobalIlluminationSpinBox | Spin Box |
| ReflectionSpinBox | Spin Box |
| AntiAliasingSpinBox | Spin Box |
| TextureSpinBox | Spin Box |
| EffectsSpinBox | Spin Box |
| FoliageSpinBox | Spin Box |
| ShadingSpinBox | Spin Box |

### Required — Audio tab (TabSwitcher page 1)
| Name | Type |
|---|---|
| MasterVolumeSlider | Slider |
| MusicVolumeSlider | Slider |
| SFXVolumeSlider | Slider |
| VoiceVolumeSlider | Slider |
| MicInputModeCombo | ComboBox (String) |
| MicDeviceCombo | ComboBox (String) |

### Required — Controls tab (TabSwitcher page 2)
| Name | Type |
|---|---|
| MouseSensitivitySlider | Slider |
| InvertYCheckBox | Check Box |
| JumpRebindButton | Button |
| JumpRebindLabel | Text |
| ShootRebindButton | Button |
| ShootRebindLabel | Text |
| SwapWeaponRebindButton | Button |
| SwapWeaponRebindLabel | Text |
| SprintRebindButton | Button |
| SprintRebindLabel | Text |

### Required — Gameplay tab (TabSwitcher page 3)
| Name | Type |
|---|---|
| SubtitlesCheckBox | Check Box |
| ColorblindModeCombo | ComboBox (String) |
| ColorblindStrengthSlider | Slider |

### Suggested hierarchy
```
Canvas Panel (root, full screen)
└─ Border "Background" (Color_Background)
   └─ Horizontal Box (fills screen)
      ├─ Size Box (Width Override 160) "Sidebar"
      │    └─ Vertical Box
      │       ├─ Overlay "GraphicsTabRow"
      │       │   ├─ Border "GraphicsTabAccent" (4px-wide bar, left-aligned, Color_Accent,
      │       │   │        Visibility toggled by code — leave Visible in Designer, code hides it)
      │       │   └─ Button "GraphicsTabButton" + Text "Graphics" (Secondary style, fills row)
      │       ├─ Overlay "AudioTabRow"     → AudioTabButton / AudioTabAccent, same pattern
      │       ├─ Overlay "ControlsTabRow"  → ControlsTabButton / ControlsTabAccent
      │       └─ Overlay "GameplayTabRow"  → GameplayTabButton / GameplayTabAccent
      └─ Vertical Box "ContentColumn" (fill remaining space)
         ├─ TabSwitcher (fills most of the column)
         │    ├─ [Page 0] Scroll Box "GraphicsPage" — one labeled row per control above
         │    ├─ [Page 1] Scroll Box "AudioPage"
         │    ├─ [Page 2] Scroll Box "ControlsPage"
         │    └─ [Page 3] Scroll Box "GameplayPage"
         └─ Horizontal Box "ButtonRow" (bottom-right aligned)
            ├─ Button "BackButton" + Text "Back" (Secondary style)
            └─ Button "ApplyButton" + Text "Apply" (Primary style)
```
**Labeled-row pattern** (repeat for every control in every tab page): a `Horizontal Box` with a
`Text` (the row label, decorative — not bound, just for readability, Body font, Color_TextMuted,
fixed width ~200 so values align) on the left, and the bound control on the right filling the
rest of the row. Rebind rows are the one exception: `JumpRebindButton` (etc.) shows "Click to
rebind" and `JumpRebindLabel` shows the current key next to it — put them side by side in the
same row, both after the decorative row label.

Combobox options and slider/spinbox ranges are populated at runtime — leave them empty/default
in the Designer.

---

## 3. WBP_Lobby — parent `UjinzzaLobbyWidget`

Per `todo.txt`'s 2026-09-02 restructure: fields grouped under **"Room"** and **"Actions"**
headings with a divider under the title, plus a separate bottom-center interaction prompt.

### Required
| Name | Type |
|---|---|
| SettingsText | Text |
| PlayerCountText | Text |
| InviteButton | Button |
| StartButton | Button |
| InteractPromptText | Text |

### Suggested hierarchy
```
Canvas Panel (root, full screen)
├─ Border "InfoPanel" (top-left anchored, panel background style, ~360px wide)
│   └─ Vertical Box
│      ├─ Text "Title" — "Lobby" (Heading font 24, Color_Accent)
│      ├─ Divider (see §0)
│      ├─ Text "RoomHeading" — "Room" (Heading font 20, Color_TextPrimary, decorative)
│      ├─ Text "SettingsText"                 ← BindWidget: SettingsText
│      │     Body font 16, Color_TextPrimary (shows room name/settings summary)
│      ├─ Text "PlayerCountText"              ← BindWidget: PlayerCountText
│      │     Body font 15, Color_TextMuted (per the 2026-09-02 note: body style, not heading)
│      ├─ Text "ActionsHeading" — "Actions" (Heading font 20, decorative)
│      ├─ Button "InviteButton" + Text "Invite Friends"   ← BindWidget: InviteButton
│      │     Secondary style
│      └─ Button "StartButton" + Text "Start Match"       ← BindWidget: StartButton
│            Primary style (host-only — code should hide/disable for non-hosts if not already)
└─ Text "InteractPromptText" (Canvas anchor 0.5,1 bottom-center, offset ~-60px up)
      ← BindWidget: InteractPromptText
      Body font 16, Color_TextPrimary, centered, Visibility: leave Visible — code shows/hides
      via SetInteractionPrompt() based on whether the text is empty
```

---

## 4. WBP_RoomSettings — parent `UjinzzaRoomSettingsWidget`

Opened by the in-world kiosk (press E). Host-editable, everyone else sees it read-only (handled
in code via `bEditable`).

### Required
| Name | Type |
|---|---|
| HeaderNote | Text |
| RoomNameBox | Editable Text Box |
| MaxPlayersSpinBox | Spin Box |
| JudgeCountSpinBox | Spin Box |
| VoteCountSpinBox | Spin Box |
| PhaseSpeedCombo | ComboBox (String) |
| RoleAssignCombo | ComboBox (String) |
| ApplyButton | Button |
| CloseButton | Button |

Ranges/options set in code: MaxPlayers 4–12, JudgeCount 1–2, VoteCount 1–3, PhaseSpeed
{Slow, Normal, Fast}, RoleAssign {Random, Host Picks} — leave Designer defaults alone.

### Suggested hierarchy
```
Canvas Panel (root)
└─ Border "Panel" (centered, ~420×480, panel background style)
   └─ Vertical Box
      ├─ Text "HeaderNote"                       ← BindWidget: HeaderNote
      │     Heading font 20, Color_Accent (e.g. "Room Settings (Host Only)" / read-only note —
      │     code likely sets this text based on bEditable, so leave it blank/placeholder)
      ├─ Divider
      ├─ [row] Text "Room Name" (decorative label) + EditableTextBox "RoomNameBox"
      ├─ [row] Text "Max Players" + SpinBox "MaxPlayersSpinBox"
      ├─ [row] Text "Judge Count" + SpinBox "JudgeCountSpinBox"
      ├─ [row] Text "Vote Count" + SpinBox "VoteCountSpinBox"
      ├─ [row] Text "Phase Speed" + ComboBoxString "PhaseSpeedCombo"
      ├─ [row] Text "Role Assign Method" + ComboBoxString "RoleAssignCombo"
      └─ Horizontal Box "ButtonRow" (bottom, right-aligned)
         ├─ Button "CloseButton" + Text "Close"   ← BindWidget: CloseButton (Secondary style)
         └─ Button "ApplyButton" + Text "Apply"   ← BindWidget: ApplyButton (Primary style)
```
Row pattern same as WBP_Settings: `Horizontal Box`, decorative label left (fixed width ~160,
Body font, Color_TextMuted), bound control right.

---

## 5. WBP_GameEnd — parent `UjinzzaGameEndWidget`

Minimal in-round overlay.

### Required
| Name | Type |
|---|---|
| EndGameButton | Button |

### Suggested hierarchy
```
Canvas Panel (root, full screen, mostly transparent so gameplay stays visible behind it)
└─ Button "EndGameButton" (Canvas anchor 0.5,1 bottom-center, offset ~-40px up)
      ← BindWidget: EndGameButton
      + child Text "End Game" — Warning style (crimson, matches its destructive/host-only
      nature — same treatment as MainMenu's Quit button), Heading font 18
```

---

## Checklist

- [ ] WBP_MainMenu — 7 required bindings placed, compiles clean
- [ ] WBP_Settings — 6 chrome + 32 per-tab bindings placed, compiles clean
- [ ] WBP_Lobby — 5 required bindings placed, compiles clean
- [ ] WBP_RoomSettings — 9 required bindings placed, compiles clean
- [ ] WBP_GameEnd — 1 required binding placed, compiles clean
- [ ] Full project compile (Ctrl+Alt+F11), then PIE `Lvl_MainMenu` — confirm no
      `LogBlueprint: Error` lines for any of the five in `Saved/Logs/jinzza.log`
