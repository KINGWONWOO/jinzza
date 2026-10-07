// Copyright Epic Games, Inc. All Rights Reserved.

#include "SjinzzaLoadingScreen.h"
#include "jinzzaUIStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"

void SjinzzaLoadingScreen::Construct(const FArguments& InArgs)
{
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));

	TSharedRef<SWidget> Title = InArgs._LogoBrush
		? StaticCastSharedRef<SWidget>(
			SNew(SBox)
			.WidthOverride(560.f)
			.HeightOverride(320.f)
			[
				SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFit)
				[
					SNew(SImage).Image(InArgs._LogoBrush)
				]
			])
		: StaticCastSharedRef<SWidget>(
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("JINZZA")))
			.Font(InArgs._TitleFont)
			.ColorAndOpacity(JinzzaUI::Sticker_White));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(WhiteBrush)
		.BorderBackgroundColor(JinzzaUI::Sticker_Ink)
		.Padding(0.f)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					Title
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 36.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCircularThrobber)
						.Radius(14.f)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(16.f, 0.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(InArgs._StatusText)
						.Font(InArgs._BodyFont)
						.ColorAndOpacity(JinzzaUI::Sticker_White)
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 18.f, 0.f, 0.f)
				[
					SNew(SBox)
					.WidthOverride(560.f)
					.HeightOverride(12.f)
					[
						SNew(SProgressBar)
						.Percent(InArgs._Progress)
						.FillColorAndOpacity(JinzzaUI::Sticker_Yellow)
					]
				]
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Bottom)
			.Padding(80.f, 0.f, 80.f, 64.f)
			[
				SNew(STextBlock)
				.Text(InArgs._Tip)
				.Font(InArgs._BodyFont)
				.ColorAndOpacity(JinzzaUI::Sticker_SubText)
				.AutoWrapText(true)
				.Justification(ETextJustify::Center)
			]
		]
	];
}
