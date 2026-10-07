// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaChatBoardWidget.h"
#include "jinzzaUIStyle.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"

void UjinzzaChatBoardWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	// Transparent - the board mesh behind it is the chalkboard surface.
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
	Root->SetBrushColor(FLinearColor::Transparent);
	Root->SetPadding(FMargin(28.f, 20.f));
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Root;

	BoardText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BoardText"));
	BoardText->SetFont(JinzzaUI::BodyFont(38));
	// Chalk: slightly warm off-white.
	BoardText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.94f, 0.88f)));
	BoardText->SetAutoWrapText(true);
	BoardText->SetJustification(ETextJustify::Center);
	Root->SetContent(BoardText);
}

void UjinzzaChatBoardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
}

void UjinzzaChatBoardWidget::SetBoardText(const FString& Text)
{
	if (BoardText)
	{
		BoardText->SetText(FText::FromString(Text));
	}
}
