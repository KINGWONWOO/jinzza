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
}
