// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaTestHudWidget.h"
#include "jinzzaInputKeys.h"
#include "jinzzaUIStyle.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"

void UjinzzaTestHudWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Top-left badge.
	UBorder* BadgeFace = nullptr;
	UOverlay* Badge = JinzzaUI::MakeSticker(WidgetTree, TEXT("Badge"), JinzzaUI::Sticker_Ink, 18.f, BadgeFace, 3.f, 5.f);
	BadgeFace->SetPadding(FMargin(16.f, 10.f));
	if (UOverlaySlot* BadgeSlot = Root->AddChildToOverlay(Badge))
	{
		BadgeSlot->SetHorizontalAlignment(HAlign_Left);
		BadgeSlot->SetVerticalAlignment(VAlign_Top);
		BadgeSlot->SetPadding(FMargin(24.f, 20.f));
	}
	UVerticalBox* BadgeStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BadgeStack"));
	BadgeFace->SetContent(BadgeStack);
	UTextBlock* BadgeTitle = JinzzaUI::MakeStickerText(WidgetTree, TEXT("BadgeTitle"), FText::FromString(TEXT("TEST MAP  테스트 맵")), 20);
	BadgeTitle->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_Yellow));
	BadgeStack->AddChildToVerticalBox(BadgeTitle);
	JinzzaUI::AddSpaced(BadgeStack, JinzzaUI::MakeStickerText(WidgetTree, TEXT("BadgeHint"),
		FText::FromString(TEXT("Walk up to a station and press the key shown.\nMatch drills (practice dummies) are at the far end.\nEnter: chat / speak on your turn")), 15, true), 4.f);

	// Bottom-center kiosk prompt: key cap + label.
	UBorder* PromptFace = nullptr;
	UOverlay* PromptPanel = JinzzaUI::MakeSticker(WidgetTree, TEXT("Prompt"), JinzzaUI::Sticker_Ink, 26.f, PromptFace);
	PromptFace->SetPadding(FMargin(12.f, 8.f, 18.f, 8.f));
	if (UOverlaySlot* PromptSlot = Root->AddChildToOverlay(PromptPanel))
	{
		PromptSlot->SetHorizontalAlignment(HAlign_Center);
		PromptSlot->SetVerticalAlignment(VAlign_Bottom);
		PromptSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 60.f));
	}
	Prompt = PromptPanel;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PromptRow"));
	PromptFace->SetContent(Row);
	UTextBlock* KeyLabel = nullptr;
	UWidget* KeyCap = JinzzaUI::MakeStickerKeyCap(WidgetTree, TEXT("PromptKeyCap"), KeyLabel, 42.f);
	PromptKeyText = KeyLabel;
	if (UHorizontalBoxSlot* KeySlot = Row->AddChildToHorizontalBox(KeyCap))
	{
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeySlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}
	PromptText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("PromptText"), FText::GetEmpty(), 24);
	if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(PromptText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UjinzzaTestHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Prompt)
	{
		Prompt->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UjinzzaTestHudWidget::SetInteractionPrompt(const FText& InPromptText)
{
	if (!Prompt || !PromptText)
	{
		return;
	}
	if (InPromptText.IsEmpty())
	{
		Prompt->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	// Re-read every time - the key may have been rebound in Settings since.
	if (PromptKeyText)
	{
		PromptKeyText->SetText(JinzzaInput::GetKeyCapText(JinzzaInput::GetBoundKey(JinzzaInput::ResolveLocalPlayer(this), JinzzaInput::GetInteractAction())));
	}
	PromptText->SetText(InPromptText);
	Prompt->SetVisibility(ESlateVisibility::HitTestInvisible);
}
