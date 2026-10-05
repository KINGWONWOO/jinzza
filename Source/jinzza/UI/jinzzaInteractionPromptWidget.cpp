// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractionPromptWidget.h"
#include "jinzzaUIStyle.h"
#include "jinzzaInputKeys.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

void UjinzzaInteractionPromptWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	// Black sticker pill (white outline + drop shadow), matching the main menu / T_Logo.
	UBorder* Face = nullptr;
	UOverlay* Root = JinzzaUI::MakeSticker(WidgetTree, TEXT("Root"), JinzzaUI::Sticker_Ink, 26.f, Face);
	WidgetTree->RootWidget = Root;
	Face->SetPadding(FMargin(12.f, 8.f, 18.f, 8.f));
	Face->SetHorizontalAlignment(HAlign_Center);
	Face->SetVerticalAlignment(VAlign_Center);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
	Face->SetContent(Row);

	UTextBlock* KeyLabel = nullptr;
	UWidget* KeyCap = JinzzaUI::MakeStickerKeyCap(WidgetTree, TEXT("KeyCap"), KeyLabel, 40.f);
	KeyText = KeyLabel;
	if (UHorizontalBoxSlot* KeySlot = Row->AddChildToHorizontalBox(KeyCap))
	{
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeySlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}

	PromptText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("PromptText"), FText::GetEmpty(), 22);
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
