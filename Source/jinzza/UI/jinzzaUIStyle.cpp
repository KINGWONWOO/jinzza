// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	UFont* LoadJinzzaUIFont()
	{
		static TWeakObjectPtr<UFont> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UFont>(nullptr, TEXT("/Game/JINZZA/Fonts/SacheonUju-Regular_Font.SacheonUju-Regular_Font"));
		}
		return Cached.Get();
	}

	/** Pill-shaped button base (source: noob-game's OptionButtonImage.png, re-tinted per style). */
	UTexture2D* LoadButtonPillTexture()
	{
		static TWeakObjectPtr<UTexture2D> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UTexture2D>(nullptr, TEXT("/Game/JINZZA/UI/Textures/T_ButtonPill.T_ButtonPill"));
		}
		return Cached.Get();
	}

	/** Pinned-note-card base (source: noob-game's MemoYellow.png, re-tinted to parchment). */
	UTexture2D* LoadNotePanelTexture()
	{
		static TWeakObjectPtr<UTexture2D> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UTexture2D>(nullptr, TEXT("/Game/JINZZA/UI/Textures/T_NotePanel.T_NotePanel"));
		}
		return Cached.Get();
	}

	/** 9-slice brush from a texture, tinted. Margins are fractions of the source image sized to
	 * protect each texture's own baked border/detail from stretch distortion. */
	FSlateBrush MakeNineSliceBrush(UTexture2D* Texture, const FMargin& Margin, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		if (Texture)
		{
			Brush.SetResourceObject(Texture);
			Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.Margin = Margin;
			Brush.TintColor = FSlateColor(Tint);
		}
		return Brush;
	}

	USoundBase* LoadButtonClickSound()
	{
		static TWeakObjectPtr<USoundBase> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/UISounds/ButtonClickPopSound_Cue.ButtonClickPopSound_Cue"));
		}
		return Cached.Get();
	}

	USoundBase* LoadButtonHoverSound()
	{
		static TWeakObjectPtr<USoundBase> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/UISounds/ButtonHover_Cue.ButtonHover_Cue"));
		}
		return Cached.Get();
	}
}

void UJinzzaUIButtonSounds::HandleClicked()
{
	if (USoundBase* Sound = LoadButtonClickSound())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void UJinzzaUIButtonSounds::HandleHovered()
{
	if (USoundBase* Sound = LoadButtonHoverSound())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

namespace JinzzaUI
{
	const FLinearColor Color_Background(0.035f, 0.030f, 0.045f, 1.0f);
	const FLinearColor Color_Panel(0.075f, 0.065f, 0.090f, 0.94f);
	const FLinearColor Color_PanelBorder(0.30f, 0.24f, 0.15f, 0.6f);
	const FLinearColor Color_Accent(0.85f, 0.68f, 0.25f, 1.0f);
	const FLinearColor Color_AccentAlt(0.75f, 0.09f, 0.12f, 1.0f);
	const FLinearColor Color_TextPrimary(0.95f, 0.95f, 0.96f, 1.0f);
	const FLinearColor Color_TextMuted(0.62f, 0.60f, 0.65f, 1.0f);

	namespace
	{
		const FLinearColor Color_ButtonNormal(0.11f, 0.095f, 0.14f, 0.95f);
		const FLinearColor Color_ButtonHovered(0.17f, 0.145f, 0.10f, 0.97f);
		const FLinearColor Color_ButtonPressed(0.22f, 0.17f, 0.08f, 1.0f);
		const FLinearColor Color_ButtonDisabled(0.08f, 0.08f, 0.09f, 0.6f);

		const FLinearColor Color_WarnButtonHovered(0.35f, 0.07f, 0.09f, 0.97f);
		const FLinearColor Color_WarnButtonPressed(0.45f, 0.05f, 0.08f, 1.0f);

		FButtonStyle MakeRoundedButtonStyle(const FLinearColor& NormalColor, const FLinearColor& HoveredColor,
			const FLinearColor& PressedColor, const FLinearColor& BorderColor)
		{
			constexpr float CornerRadius = 6.f;
			constexpr float OutlineWidth = 1.5f;

			FButtonStyle Style;
			if (UTexture2D* Pill = LoadButtonPillTexture())
			{
				// T_ButtonPill is 835x312 with its own baked outline/shine; the rounded caps eat
				// ~19% of the width, the outline stroke ~8% of the height - BorderColor isn't used
				// here since the outline is baked into the art, not drawn separately.
				const FMargin PillMargin(0.19f, 0.08f, 0.19f, 0.08f);
				Style.Normal = MakeNineSliceBrush(Pill, PillMargin, NormalColor);
				Style.Hovered = MakeNineSliceBrush(Pill, PillMargin, HoveredColor);
				Style.Pressed = MakeNineSliceBrush(Pill, PillMargin, PressedColor);
				Style.Disabled = MakeNineSliceBrush(Pill, PillMargin, Color_ButtonDisabled);
			}
			else
			{
				Style.Normal = FSlateRoundedBoxBrush(NormalColor, CornerRadius, BorderColor, OutlineWidth);
				Style.Hovered = FSlateRoundedBoxBrush(HoveredColor, CornerRadius, BorderColor, OutlineWidth);
				Style.Pressed = FSlateRoundedBoxBrush(PressedColor, CornerRadius, BorderColor, OutlineWidth);
				Style.Disabled = FSlateRoundedBoxBrush(Color_ButtonDisabled, CornerRadius, FLinearColor(0.f, 0.f, 0.f, 0.f), 0.f);
			}
			Style.NormalPadding = FMargin(16.f, 10.f);
			Style.PressedPadding = FMargin(16.f, 11.f, 16.f, 9.f);
			return Style;
		}

		UButton* MakeStyledButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize,
			const FButtonStyle& Style, const FLinearColor& TextColor)
		{
			UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
			Button->SetStyle(Style);

			UTextBlock* ButtonText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Label")));
			ButtonText->SetText(Label);
			ButtonText->SetFont(HeadingFont(FMath::RoundToInt(FontSize)));
			ButtonText->SetJustification(ETextJustify::Center);
			ButtonText->SetColorAndOpacity(FSlateColor(TextColor));

			UJinzzaUIButtonSounds* SoundBinder = NewObject<UJinzzaUIButtonSounds>(Button);
			Button->OnClicked.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleClicked);
			Button->OnHovered.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleHovered);

			Button->AddChild(ButtonText);
			return Button;
		}
	}

	FSlateFontInfo TitleFont(int32 Size)
	{
		if (UFont* Font = LoadJinzzaUIFont())
		{
			return FSlateFontInfo(Font, Size);
		}
		return FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}

	FSlateFontInfo HeadingFont(int32 Size)
	{
		if (UFont* Font = LoadJinzzaUIFont())
		{
			return FSlateFontInfo(Font, Size);
		}
		return FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}

	FSlateFontInfo BodyFont(int32 Size)
	{
		if (UFont* Font = LoadJinzzaUIFont())
		{
			return FSlateFontInfo(Font, Size);
		}
		return FCoreStyle::GetDefaultFontStyle("Regular", Size);
	}

	UButton* MakePrimaryButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize)
	{
		static const FButtonStyle Style = MakeRoundedButtonStyle(Color_ButtonNormal, Color_ButtonHovered, Color_ButtonPressed, Color_Accent);
		return MakeStyledButton(Tree, Name, Label, FontSize, Style, Color_TextPrimary);
	}

	UButton* MakeSecondaryButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize)
	{
		static const FButtonStyle Style = MakeRoundedButtonStyle(
			FLinearColor(0.09f, 0.09f, 0.10f, 0.7f),
			FLinearColor(0.14f, 0.14f, 0.16f, 0.85f),
			FLinearColor(0.18f, 0.18f, 0.20f, 0.95f),
			Color_TextMuted);
		return MakeStyledButton(Tree, Name, Label, FontSize, Style, Color_TextMuted);
	}

	UButton* MakeWarningButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize)
	{
		static const FButtonStyle Style = MakeRoundedButtonStyle(Color_ButtonNormal, Color_WarnButtonHovered, Color_WarnButtonPressed, Color_AccentAlt);
		return MakeStyledButton(Tree, Name, Label, FontSize, Style, Color_TextPrimary);
	}

	UBorder* MakePanelBackground(UWidgetTree* Tree, FName Name)
	{
		UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		FSlateBrush Brush = FSlateRoundedBoxBrush(Color_Panel, 10.f, Color_PanelBorder, 1.5f);
		Panel->SetBrush(Brush);
		return Panel;
	}

	UBorder* MakeNoteBackground(UWidgetTree* Tree, FName Name)
	{
		UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		if (UTexture2D* Note = LoadNotePanelTexture())
		{
			// T_NotePanel is 1088x960: a folded corner (bottom-right) and washi tape overlapping the
			// top edge, so the margins are asymmetric to keep both un-stretched.
			const FMargin NoteMargin(0.22f, 0.30f, 0.22f, 0.22f);
			// Warm parchment tint, close to the source art's own cream rather than a heavy recolor -
			// the aged-note look already reads as "evidence/case file" against the dark noir panels.
			const FLinearColor NoteTint(0.90f, 0.82f, 0.60f, 0.97f);
			Panel->SetBrush(MakeNineSliceBrush(Note, NoteMargin, NoteTint));
		}
		else
		{
			Panel->SetBrush(FSlateRoundedBoxBrush(Color_Panel, 10.f, Color_PanelBorder, 1.5f));
		}
		return Panel;
	}

	UWidget* MakeDivider(UWidgetTree* Tree, FName Name, float Width, float Height)
	{
		UBorder* Bar = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Bar")));
		Bar->SetBrush(FSlateRoundedBoxBrush(Color_Accent, Height * 0.5f));
		Bar->SetPadding(FMargin(0.f));

		USizeBox* SizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		SizeBox->SetWidthOverride(Width);
		SizeBox->SetHeightOverride(Height);
		SizeBox->AddChild(Bar);
		return SizeBox;
	}

	UTextBlock* MakeTitleText(UWidgetTree* Tree, FName Name, const FText& Text, int32 Size)
	{
		UTextBlock* Title = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Title->SetText(Text);
		Title->SetFont(TitleFont(Size));
		Title->SetJustification(ETextJustify::Center);
		Title->SetColorAndOpacity(FSlateColor(Color_Accent));
		return Title;
	}

	UTextBlock* MakeSectionHeading(UWidgetTree* Tree, FName Name, const FText& Text)
	{
		UTextBlock* Heading = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Heading->SetText(Text);
		Heading->SetFont(HeadingFont(20));
		Heading->SetColorAndOpacity(FSlateColor(Color_TextPrimary));
		return Heading;
	}

	UTextBlock* MakeBodyText(UWidgetTree* Tree, FName Name, const FText& Text, bool bMuted)
	{
		UTextBlock* Body = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Body->SetText(Text);
		Body->SetFont(BodyFont(15));
		Body->SetColorAndOpacity(FSlateColor(bMuted ? Color_TextMuted : Color_TextPrimary));
		return Body;
	}
}
