// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractionPromptWidget.h"
#include "jinzzaUIStyle.h"
#include "jinzzaInputKeys.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

void UjinzzaInteractionPromptWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Root = JinzzaUI::MakeNoteBackground(WidgetTree, TEXT("Root"));
	WidgetTree->RootWidget = Root;
	Root->SetPadding(FMargin(16.f, 10.f));
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Center);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	Root->SetContent(Row);

	UTextBlock* KeyLabel = nullptr;
	UWidget* KeyCap = JinzzaUI::MakeKeyCap(WidgetTree, TEXT("KeyCap"), KeyLabel, 30.f);
	KeyText = KeyLabel;
	if (UHorizontalBoxSlot* KeySlot = Row->AddChildToHorizontalBox(KeyCap))
	{
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeySlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}

	PromptText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("PromptText"), FText::GetEmpty());
	if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(PromptText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UjinzzaInteractionPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
}

void UjinzzaInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshBoundKey();
}

void UjinzzaInteractionPromptWidget::SetPrompt(const FText& Text)
{
	if (PromptText)
	{
		PromptText->SetText(Text);
	}
}

void UjinzzaInteractionPromptWidget::RefreshBoundKey()
{
	if (KeyText)
	{
		const FKey Key = JinzzaInput::GetBoundKey(JinzzaInput::ResolveLocalPlayer(this), JinzzaInput::GetInteractAction());
		KeyText->SetText(JinzzaInput::GetKeyCapText(Key));
	}
}
