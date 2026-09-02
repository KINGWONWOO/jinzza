// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"

class UButton;
class UWidget;
class UWidgetTree;
class UTextBlock;
class UBorder;

/**
 * Shared visual language for JINZZA's hand-built C++ UI (no Widget Blueprint assets/UMG
 * Designer available in this project - see UjinzzaMainMenuWidget's header comment). A dark,
 * noir-interrogation-room palette (near-black panels, crimson "guilty" / gold "judge" accents)
 * fitting the game's courtroom/social-deduction theme, applied consistently across the main
 * menu, lobby, and settings screens via FSlateRoundedBoxBrush so it needs no texture assets.
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

	/** Thin accent-gold divider bar, e.g. under a title. Returns the sized wrapper widget to add to a panel. */
	UWidget* MakeDivider(UWidgetTree* Tree, FName Name, float Width = 120.f, float Height = 3.f);

	UTextBlock* MakeTitleText(UWidgetTree* Tree, FName Name, const FText& Text, int32 Size = 48);
	UTextBlock* MakeSectionHeading(UWidgetTree* Tree, FName Name, const FText& Text);
	UTextBlock* MakeBodyText(UWidgetTree* Tree, FName Name, const FText& Text, bool bMuted = false);
}
