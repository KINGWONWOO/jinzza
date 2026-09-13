// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "jinzzaUIStyle.generated.h"

class UButton;
class UWidget;
class UWidgetTree;
class UTextBlock;
class UBorder;
class UVerticalBox;
class UVerticalBoxSlot;

/** Binds click/hover sound playback to buttons built via JinzzaUI::MakeStyledButton. Not for direct use. */
UCLASS()
class UJinzzaUIButtonSounds : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();
};

/**
 * Shared visual language for JINZZA's C++ UI toolkit. A dark, noir-interrogation-room palette
 * (near-black panels, crimson "guilty" / gold "judge" accents) fitting the game's courtroom/
 * social-deduction theme, applied consistently across the main menu, lobby, and settings
 * screens.
 *
 * Buttons (MakePrimaryButton/MakeSecondaryButton/MakeWarningButton) and the pinned-note panel
 * (MakeNoteBackground) are built from two texture assets under /Game/JINZZA/UI/Textures -
 * T_ButtonPill and T_NotePanel - re-tinted into this palette from source art referenced out of
 * the noob-game project's UI folder (a pastel/cute style; only the paper-note-card and pill-
 * button *shapes* were borrowed, tinted dark/gold/crimson to fit here rather than copied as-is).
 * MakePanelBackground (the large "room wall" panel surface) stays a plain FSlateRoundedBoxBrush
 * with no texture, so the two aren't visually competing. If either texture is missing, every
 * helper below falls back to its original flat FSlateRoundedBoxBrush look.
 *
 * Text uses the SacheonUju Korean font asset (falls back to the engine default if missing);
 * every button built here also gets a shared click/hover sound via UJinzzaUIButtonSounds.
 *
 * These Make* helpers aren't called by any widget class yet - UjinzzaMainMenuWidget etc. are
 * UMG-authored (BindWidget from a Widget Blueprint you build in the Designer), so this file is
 * this project's reference for that: T_ButtonPill/T_NotePanel and the colors/fonts below are
 * what to plug into each WBP's Brush/Appearance fields in the Designer to match. A C++-built
 * widget (like AjinzzaRoomSettingsKiosk's world-space label) can also call these directly.
 */
namespace JinzzaUI
{
	extern const FLinearColor Color_Background;
	extern const FLinearColor Color_Panel;
	extern const FLinearColor Color_PanelBorder;
	extern const FLinearColor Color_Accent;
	extern const FLinearColor Color_AccentAlt;
	extern const FLinearColor Color_TextPrimary;
	extern const FLinearColor Color_TextMuted;

	FSlateFontInfo TitleFont(int32 Size = 48);
	FSlateFontInfo HeadingFont(int32 Size = 24);
	FSlateFontInfo BodyFont(int32 Size = 16);

	/** Accent-gold call-to-action button (Host Game, Apply, Create Room, Start Match, ...). */
	UButton* MakePrimaryButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize = 22.f);

	/** Muted outline button for secondary actions (Back, Cancel, Quit, tab headers). */
	UButton* MakeSecondaryButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize = 18.f);

	/** Crimson-tinted button for destructive/warning actions (Quit, Leave Room). */
	UButton* MakeWarningButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize = 18.f);

	/** Pastel-tinted pill button matching the NOOB-GAME reference main menu's button column (see
	 * docs/Unreal_Game_Noob-main .../Doc/Images/MainMenu.png) - same T_ButtonPill shape as
	 * MakePrimaryButton/etc. but with a caller-supplied tint per button instead of the shared noir
	 * palette, plus a small white circular icon-slot badge to the left of the label standing in for
	 * NOOB's per-button icon (controller/gear/magnifier/power) until real icon art exists - swap
	 * the badge for a UImage at that point. Fallback used only for main-menu buttons with no
	 * matching NOOB-GAME source art (see MakeNoobIconButton) - every other panel keeps the noir
	 * MakePrimaryButton/MakeSecondaryButton/MakeWarningButton look. */
	UButton* MakeMenuActionButton(UWidgetTree* Tree, FName Name, const FText& Label, const FLinearColor& TintColor, float FontSize = 20.f);

	/** Main-menu button built from an actual NOOB-GAME button image (a fully baked pill + border +
	 * icon PNG from the reference project's own UI source folder - e.g. T_ButtonHost, imported
	 * from .../Unreal_Game_Noob-main/기획/자료/Menu/Hostbutton.png - see jinzzaMainMenuWidget.cpp
	 * for the full set), drawn at the image's own aspect ratio rather than nine-sliced since the
	 * icon's position within the art is fixed. The label sits to the right of the baked icon (an
	 * invisible spacer reserves that space) rather than centered, matching NOOB-GAME's own text
	 * placement. Hover/press feedback is a brightness tint of the same single baked image, since
	 * there's only one piece of source art per button (no separate hover/press art in NOOB-GAME's
	 * source folder either). Falls back to MakeMenuActionButton's plain tinted pill if TexturePath
	 * fails to load (e.g. the asset was never imported), so a missing texture degrades gracefully
	 * instead of producing an invisible button. */
	UButton* MakeNoobIconButton(UWidgetTree* Tree, FName Name, const FText& Label, const TCHAR* TexturePath, const FLinearColor& FallbackTintColor, float Height = 88.f, float FontSize = 20.f);

	/** Generic circular icon button shell: a solid-tinted circle (plain FSlateRoundedBoxBrush at
	 * radius = half the diameter, no source art needed for the circle itself) wrapping IconContent
	 * (centered, may be null for a blank circle), with a caption label below the circle rather than
	 * beside it. The button's own style brush is fully transparent (ESlateBrushDrawType::NoDrawType)
	 * - the visible circle is a child UBorder, not the button background - so the caption sits on
	 * the panel behind it with no button-colored rectangle showing through. Shared by
	 * MakeNoobCircleIconButton (icon = an image) and callers that build a from-primitives icon
	 * widget directly (e.g. MakeMaskIcon/MakeMicIcon below) - see jinzzaMainMenuWidget.cpp. */
	UButton* MakeCircleIconButton(UWidgetTree* Tree, FName Name, const FText& Label, UWidget* IconContent, const FLinearColor& TintColor, float Diameter = 88.f, float FontSize = 15.f);

	/** Circular icon button built from a NOOB-GAME sourced icon image (aspect-fit within the
	 * circle, not stretched to a square) - see MakeCircleIconButton for the shared shell. Falls
	 * back to a blank tinted circle (no icon) if TexturePath fails to load. */
	UButton* MakeNoobCircleIconButton(UWidgetTree* Tree, FName Name, const FText& Label, const TCHAR* TexturePath, const FLinearColor& TintColor, float Diameter = 88.f, float FontSize = 15.f);

	/** Simple masquerade-mask icon (a rounded bar "face" with two dark eye-hole cutouts) built
	 * entirely from FSlateRoundedBoxBrush primitives - no source art needed. Evokes JINZZA's
	 * "Imitator" disguise premise directly, unlike any borrowed NOOB-GAME art; used for the
	 * Customize button and the top-left logo badge. */
	UWidget* MakeMaskIcon(UWidgetTree* Tree, FName Name, float Size, const FLinearColor& MaskColor);

	/** Simple microphone icon (a capsule head over a thin stand and base) built entirely from
	 * FSlateRoundedBoxBrush primitives - no source art needed. Used for the Voice Test button. */
	UWidget* MakeMicIcon(UWidgetTree* Tree, FName Name, float Size, const FLinearColor& MicColor);

	/** Rounded, semi-opaque panel background with a faint border. */
	UBorder* MakePanelBackground(UWidgetTree* Tree, FName Name);

	/** Small "pinned case note" accent panel (T_NotePanel, tinted parchment) - for a heading callout
	 * or a note-like aside, not for large panel surfaces. Falls back to MakePanelBackground's look
	 * if T_NotePanel is missing. */
	UBorder* MakeNoteBackground(UWidgetTree* Tree, FName Name);

	/** Thin accent-gold divider bar, e.g. under a title. Returns the sized wrapper widget to add to a panel. */
	UWidget* MakeDivider(UWidgetTree* Tree, FName Name, float Width = 120.f, float Height = 3.f);

	UTextBlock* MakeTitleText(UWidgetTree* Tree, FName Name, const FText& Text, int32 Size = 48);
	UTextBlock* MakeSectionHeading(UWidgetTree* Tree, FName Name, const FText& Text);
	UTextBlock* MakeBodyText(UWidgetTree* Tree, FName Name, const FText& Text, bool bMuted = false);

	/** A labeled settings row: a fixed-width muted label on the left, the given control filling
	 * the rest on the right. Used by every C++-built settings/kiosk-style panel to avoid
	 * repeating this layout per control - see UjinzzaSettingsWidget/UjinzzaRoomSettingsWidget. */
	UWidget* MakeLabeledRow(UWidgetTree* Tree, FName Name, const FText& LabelText, UWidget* Control, float LabelWidth = 160.f);

	/** Adds Child to the bottom of Box with TopPadding above it, filled horizontally - the
	 * standard vertical-stack spacing helper every C++-built panel needs. Centralized here
	 * (rather than each widget .cpp defining its own identically-named anonymous-namespace
	 * copy, as several used to) specifically because Unreal's unity build concatenates multiple
	 * .cpp files into one translation unit - two same-named functions in anonymous namespaces
	 * only collide once unity grouping happens to put both files in the same blob, which is a
	 * latent, unity-grouping-dependent bug rather than an immediate one. */
	UVerticalBoxSlot* AddSpaced(UVerticalBox* Box, UWidget* Child, float TopPadding = 10.f);
}
