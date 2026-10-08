// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaChatBoardWidget.h"
#include "jinzzaSketchPad.h"
#include "jinzzaQuestionTypes.h"
#include "jinzzaUIStyle.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
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

	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Layers"));
	Root->SetContent(Layers);

	BoardText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BoardText"));
	BoardText->SetFont(JinzzaUI::BodyFont(38));
	// Chalk: slightly warm off-white.
	BoardText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.94f, 0.88f)));
	BoardText->SetAutoWrapText(true);
	BoardText->SetJustification(ETextJustify::Center);
	if (UOverlaySlot* TextSlot = Layers->AddChildToOverlay(BoardText))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	// Question Time answer: a 4:3 sketchbook page over the board face.
	SketchBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SketchBox"));
	SketchBox->SetWidthOverride(416.f);
	SketchBox->SetHeightOverride(312.f);
	SketchBox->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* SketchSlot = Layers->AddChildToOverlay(SketchBox))
	{
		SketchSlot->SetHorizontalAlignment(HAlign_Center);
		SketchSlot->SetVerticalAlignment(VAlign_Center);
	}
	UOverlay* SketchLayers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SketchLayers"));
	SketchBox->AddChild(SketchLayers);

	Sketch = WidgetTree->ConstructWidget<UjinzzaSketchPad>(UjinzzaSketchPad::StaticClass(), TEXT("Sketch"));
	if (UOverlaySlot* PadSlot = SketchLayers->AddChildToOverlay(Sketch))
	{
		PadSlot->SetHorizontalAlignment(HAlign_Fill);
		PadSlot->SetVerticalAlignment(VAlign_Fill);
	}

	NoAnswerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NoAnswerText"));
	NoAnswerText->SetText(FText::FromString(TEXT("?")));
	NoAnswerText->SetFont(JinzzaUI::BodyFont(160));
	NoAnswerText->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_Ink));
	NoAnswerText->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* NoAnswerSlot = SketchLayers->AddChildToOverlay(NoAnswerText))
	{
		NoAnswerSlot->SetHorizontalAlignment(HAlign_Center);
		NoAnswerSlot->SetVerticalAlignment(VAlign_Center);
	}
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

void UjinzzaChatBoardWidget::SetSketch(bool bShow, const FJinzzaDrawing* Drawing)
{
	if (!SketchBox || !Sketch || !NoAnswerText)
	{
		return;
	}

	SketchBox->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Sketch->SetDrawing(Drawing ? *Drawing : FJinzzaDrawing());
	NoAnswerText->SetVisibility(bShow && Drawing && Drawing->IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
