// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaPropUsageWidget.h"
#include "jinzzaUIStyle.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"

void UjinzzaPropUsageWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = JinzzaUI::MakeNoteBackground(WidgetTree, TEXT("Panel"));
	Panel->SetPadding(FMargin(16.f, 10.f));

	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Content"));
	Panel->SetContent(Content);

	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IconBox"));
	IconBox->SetWidthOverride(40.f);
	IconBox->SetHeightOverride(40.f);

	UsageIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("UsageIcon"));
	UsageIcon->SetVisibility(ESlateVisibility::Collapsed);
	IconBox->SetContent(UsageIcon);

	if (UHorizontalBoxSlot* IconSlot = Content->AddChildToHorizontalBox(IconBox))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}

	UsageText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("UsageText"), FText::GetEmpty());
	if (UHorizontalBoxSlot* TextSlot = Content->AddChildToHorizontalBox(UsageText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Right);
		PanelSlot->SetVerticalAlignment(VAlign_Bottom);
		PanelSlot->SetPadding(FMargin(0.f, 0.f, 40.f, 40.f));
	}
}

void UjinzzaPropUsageWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
}

void UjinzzaPropUsageWidget::SetPropInfo(UTexture2D* Icon, const FText& Description)
{
	if (UsageIcon)
	{
		UsageIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Icon)
		{
			UsageIcon->SetBrushFromTexture(Icon);
		}
	}

	if (UsageText)
	{
		UsageText->SetText(Description);
	}
}
