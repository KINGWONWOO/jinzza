// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaWorldSignWidget.h"
#include "jinzzaUIStyle.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"

void UjinzzaWorldSignWidget::BuildWidgetTree()
{
	// A Widget Blueprint subclass brings its own layout.
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Face = nullptr;
	UOverlay* Sticker = JinzzaUI::MakeSticker(WidgetTree, TEXT("Sign"), JinzzaUI::Sticker_Ink, 24.f, Face, 5.f, 8.f);
	Face->SetPadding(FMargin(28.f, 16.f));
	Face->SetHorizontalAlignment(HAlign_Center);
	Face->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Sticker;

	SignText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SignText"));
	SignText->SetFont(JinzzaUI::BodyFont(26));
	SignText->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_White));
	SignText->SetAutoWrapText(true);
	SignText->SetJustification(ETextJustify::Center);
	Face->SetContent(SignText);
}

void UjinzzaWorldSignWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildWidgetTree();
}

void UjinzzaWorldSignWidget::SetSignText(const FText& Text)
{
	if (SignText)
	{
		SignText->SetText(Text);
	}
}
