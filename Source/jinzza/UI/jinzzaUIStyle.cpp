// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"

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
			Style.Normal = FSlateRoundedBoxBrush(NormalColor, CornerRadius, BorderColor, OutlineWidth);
			Style.Hovered = FSlateRoundedBoxBrush(HoveredColor, CornerRadius, BorderColor, OutlineWidth);
			Style.Pressed = FSlateRoundedBoxBrush(PressedColor, CornerRadius, BorderColor, OutlineWidth);
			Style.Disabled = FSlateRoundedBoxBrush(Color_ButtonDisabled, CornerRadius, FLinearColor(0.f, 0.f, 0.f, 0.f), 0.f);
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

			Button->AddChild(ButtonText);
			return Button;
		}
	}

	FSlateFontInfo TitleFont(int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}

	FSlateFontInfo HeadingFont(int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}

	FSlateFontInfo BodyFont(int32 Size)
	{
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
