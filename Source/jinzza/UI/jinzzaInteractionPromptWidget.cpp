// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractionPromptWidget.h"
#include "jinzzaUIStyle.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"

void UjinzzaInteractionPromptWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Root = JinzzaUI::MakeNoteBackground(WidgetTree, TEXT("Root"));
	WidgetTree->RootWidget = Root;
	Root->SetPadding(FMargin(24.f, 12.f));
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Center);

	PromptText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("PromptText"), FText::GetEmpty());
	PromptText->SetJustification(ETextJustify::Center);
	Root->SetContent(PromptText);
}

void UjinzzaInteractionPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
}

void UjinzzaInteractionPromptWidget::SetPrompt(const FText& Text)
{
	if (PromptText)
	{
		PromptText->SetText(Text);
	}
}
