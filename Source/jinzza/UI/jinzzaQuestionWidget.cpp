// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaQuestionWidget.h"
#include "jinzzaSketchPad.h"
#include "jinzzaChatBoardComponent.h"
#include "jinzzaGameGameState.h"
#include "jinzzaGamePlayerController.h"
#include "jinzzaPartyPlayerState.h"
#include "jinzzaUIStyle.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Sound/SoundBase.h"

namespace
{
	constexpr int32 MaxQuestionChars = 100;

	/** Pixel size of one split-screen camera feed (4:3); smaller once there are many seats. */
	constexpr int32 PanelCaptureWidth = 640;
	constexpr int32 PanelCaptureHeight = 480;
	constexpr int32 PanelCaptureWidthSmall = 480;
	constexpr int32 PanelCaptureHeightSmall = 360;
	constexpr float PanelCameraFOV = 52.f;

	/** The question sign drops this far (px) from above, then bounces and swings. */
	constexpr float SignDropDistance = 460.f;
	constexpr float SignDropSeconds = 0.9f;
	/** While Revealing the sign is shown bigger, then shrinks to its resting size just before answering. */
	constexpr float SignRevealScale = 1.3f;

	/** TEMP placeholder sounds (same Unreal_Game_Noob set as the rest of the project's temp audio). */
	const TCHAR* const SoundSignDrop = TEXT("/Game/JINZZA/Audio/Sounds/UISounds/LobbyPannelOpen__cut_1sec_.LobbyPannelOpen__cut_1sec_");
	const TCHAR* const SoundTick = TEXT("/Game/JINZZA/Audio/Sounds/UISounds/ButtonHover.ButtonHover");
	const TCHAR* const SoundBoardFlip = TEXT("/Game/JINZZA/Audio/Sounds/UISounds/ButtonClickPopSound.ButtonClickPopSound");
	const TCHAR* const SoundSubmitted = TEXT("/Game/JINZZA/Audio/Sounds/BasketballHoop/correctanswer.correctanswer");
	const TCHAR* const SoundTimeUp = TEXT("/Game/JINZZA/Audio/Sounds/Emotes/WrongAnswerSound.WrongAnswerSound");

	float QuestionBounceOut(float X)
	{
		constexpr float N = 7.5625f;
		constexpr float D = 2.75f;
		if (X < 1.f / D)
		{
			return N * X * X;
		}
		if (X < 2.f / D)
		{
			X -= 1.5f / D;
			return N * X * X + 0.75f;
		}
		if (X < 2.5f / D)
		{
			X -= 2.25f / D;
			return N * X * X + 0.9375f;
		}
		X -= 2.625f / D;
		return N * X * X + 0.984375f;
	}

	FButtonStyle MakeSwatchStyle(const FLinearColor& Color, bool bSelected)
	{
		const FLinearColor Outline = bSelected ? JinzzaUI::Sticker_Yellow : JinzzaUI::Sticker_White;
		const float Width = bSelected ? 6.f : 3.f;
		FButtonStyle Style;
		Style.SetNormal(FSlateRoundedBoxBrush(Color, 23.f, Outline, Width));
		Style.SetHovered(FSlateRoundedBoxBrush(Color, 23.f, JinzzaUI::Sticker_Yellow, Width + 1.f));
		Style.SetPressed(FSlateRoundedBoxBrush(Color * 0.85f, 23.f, JinzzaUI::Sticker_Yellow, Width + 1.f));
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		return Style;
	}

	int32 QuestionGridColumns(int32 Count)
	{
		return Count <= 2 ? FMath::Max(1, Count) : Count <= 4 ? 2 : Count <= 9 ? 3 : 4;
	}
}

void UJinzzaQuestionToolHandler::HandleClicked()
{
	if (UjinzzaQuestionWidget* Owner = OwnerWidget.Get())
	{
		Owner->HandleTool(Tool, Index);
	}
}

// --- Build ----------------------------------------------------------------------------------------

void UjinzzaQuestionWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	auto AddFull = [Root](UWidget* Child)
	{
		if (UOverlaySlot* FullSlot = Root->AddChildToOverlay(Child))
		{
			FullSlot->SetHorizontalAlignment(HAlign_Fill);
			FullSlot->SetVerticalAlignment(VAlign_Fill);
		}
	};

	// Split screen: a dark backdrop and the camera panels below the sign.
	UOverlay* Split = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SplitLayer"));
	UBorder* SplitBack = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SplitBack"));
	SplitBack->SetBrushColor(FLinearColor(0.025f, 0.025f, 0.035f, 1.f));
	if (UOverlaySlot* BackSlot = Split->AddChildToOverlay(SplitBack))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetVerticalAlignment(VAlign_Fill);
	}
	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("Grid"));
	Grid->SetSlotPadding(FMargin(8.f));
	if (UOverlaySlot* GridSlot = Split->AddChildToOverlay(Grid))
	{
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
		GridSlot->SetPadding(FMargin(20.f, 150.f, 20.f, 20.f));
	}
	SplitLayer = Split;
	AddFull(Split);

	AnswerLayer = BuildAnswerScreen();
	AddFull(AnswerLayer);

	AskLayer = BuildAskPanel();
	AddFull(AskLayer);

	// Top banner while the Judge is asking (the sign isn't up yet).
	UBorder* BannerFace = nullptr;
	UOverlay* BannerSticker = JinzzaUI::MakeSticker(WidgetTree, TEXT("Banner"), JinzzaUI::Sticker_Ink, 24.f, BannerFace);
	BannerFace->SetPadding(FMargin(28.f, 12.f));
	BannerText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("BannerText"), FText::GetEmpty(), 26);
	BannerText->SetJustification(ETextJustify::Center);
	BannerFace->SetContent(BannerText);
	Banner = BannerSticker;
	if (UOverlaySlot* BannerSlot = Root->AddChildToOverlay(BannerSticker))
	{
		BannerSlot->SetHorizontalAlignment(HAlign_Center);
		BannerSlot->SetVerticalAlignment(VAlign_Top);
		BannerSlot->SetPadding(FMargin(0.f, 40.f, 0.f, 0.f));
	}

	SignRoot = BuildSign();
	if (UOverlaySlot* SignSlot = Root->AddChildToOverlay(SignRoot))
	{
		SignSlot->SetHorizontalAlignment(HAlign_Center);
		SignSlot->SetVerticalAlignment(VAlign_Top);
		SignSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));
	}
}

UWidget* UjinzzaQuestionWidget::BuildSign()
{
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SignRoot"));

	// The sign: a white board with a thick ink outline - "Q." in coral, the question in big ink letters.
	UBorder* Face = nullptr;
	UOverlay* SignSticker = JinzzaUI::MakeSticker(WidgetTree, TEXT("Sign"), JinzzaUI::Sticker_White, 22.f, Face, 5.f, 8.f);
	Face->SetBrushColor(FLinearColor::White);
	Face->SetPadding(FMargin(30.f, 16.f, 36.f, 18.f));
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SignLine"));
	Face->SetContent(Line);

	UTextBlock* QMark = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SignQ"));
	QMark->SetText(FText::FromString(TEXT("Q.")));
	QMark->SetFont(JinzzaUI::TitleFont(48));
	QMark->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_Coral));
	if (UHorizontalBoxSlot* QSlot = Line->AddChildToHorizontalBox(QMark))
	{
		QSlot->SetVerticalAlignment(VAlign_Center);
		QSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}

	USizeBox* TextWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SignTextWidth"));
	TextWidth->SetMaxDesiredWidth(1000.f);
	SignText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SignText"));
	SignText->SetFont(JinzzaUI::BodyFont(34));
	SignText->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_Ink));
	SignText->SetAutoWrapText(true);
	TextWidth->AddChild(SignText);
	if (UHorizontalBoxSlot* TextSlot = Line->AddChildToHorizontalBox(TextWidth))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	Sign = SignSticker;
	Sign->SetRenderTransformPivot(FVector2D(0.5f, 0.f));
	if (UVerticalBoxSlot* SignSlot = Column->AddChildToVerticalBox(SignSticker))
	{
		SignSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// Countdown pill under the sign.
	UBorder* CountdownBorder = nullptr;
	UOverlay* CountdownSticker = JinzzaUI::MakeSticker(WidgetTree, TEXT("Countdown"), JinzzaUI::Sticker_Ink, 20.f, CountdownBorder);
	CountdownFace = CountdownBorder;
	CountdownFace->SetPadding(FMargin(24.f, 6.f));
	CountdownText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("CountdownText"), FText::GetEmpty(), 26);
	CountdownText->SetJustification(ETextJustify::Center);
	CountdownFace->SetContent(CountdownText);
	Countdown = CountdownSticker;
	Countdown->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	if (UVerticalBoxSlot* CountdownSlot = Column->AddChildToVerticalBox(CountdownSticker))
	{
		CountdownSlot->SetHorizontalAlignment(HAlign_Center);
		CountdownSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}
	return Column;
}

UWidget* UjinzzaQuestionWidget::BuildAskPanel()
{
	UOverlay* Layer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("AskLayer"));

	UBorder* Face = nullptr;
	UOverlay* Panel = JinzzaUI::MakeStickerPanel(WidgetTree, TEXT("AskPanel"), Face);
	if (UOverlaySlot* PanelSlot = Layer->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Bottom);
		PanelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 56.f));
	}

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AskStack"));
	Face->SetContent(Stack);
	Stack->AddChildToVerticalBox(JinzzaUI::MakeStickerHeading(WidgetTree, TEXT("AskHeading"), FText::FromString(TEXT("Ask the candidates!")), 32));
	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeStickerText(WidgetTree, TEXT("AskHint"),
		FText::FromString(TEXT("They answer by drawing on their boards. Everyone can talk while you type.")), 18, true), 4.f);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AskRow"));
	JinzzaUI::AddSpaced(Stack, Row, 16.f);

	USizeBox* BoxWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AskBoxWidth"));
	BoxWidth->SetWidthOverride(720.f);
	QuestionBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("QuestionBox"));
	JinzzaUI::ApplyStickerStyle(QuestionBox);
	QuestionBox->SetHintText(FText::FromString(TEXT("e.g. Draw what you had for breakfast")));
	QuestionBox->OnTextCommitted.AddDynamic(this, &UjinzzaQuestionWidget::HandleQuestionCommitted);
	QuestionBox->OnTextChanged.AddDynamic(this, &UjinzzaQuestionWidget::HandleQuestionChanged);
	BoxWidth->AddChild(QuestionBox);
	if (UHorizontalBoxSlot* BoxSlot = Row->AddChildToHorizontalBox(BoxWidth))
	{
		BoxSlot->SetVerticalAlignment(VAlign_Center);
	}

	AskButton = MakeTool(Row, FText::FromString(TEXT("Ask!")), JinzzaUI::Sticker_Yellow, EJinzzaQuestionTool::SubmitQuestion, 0, 24.f);
	JinzzaUI::SetStickerButtonSelected(AskButton, JinzzaUI::Sticker_Yellow, true);

	AskCounter = JinzzaUI::MakeStickerText(WidgetTree, TEXT("AskCounter"), FText::FromString(FString::Printf(TEXT("0/%d"), MaxQuestionChars)), 16, true);
	if (UVerticalBoxSlot* CounterSlot = JinzzaUI::AddSpaced(Stack, AskCounter, 6.f))
	{
		CounterSlot->SetHorizontalAlignment(HAlign_Right);
	}
	return Layer;
}

UWidget* UjinzzaQuestionWidget::BuildAnswerScreen()
{
	UOverlay* Layer = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("AnswerLayer"));

	UBorder* Back = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AnswerBack"));
	Back->SetBrushColor(FLinearColor(0.06f, 0.08f, 0.11f, 1.f));
	if (UOverlaySlot* BackSlot = Layer->AddChildToOverlay(Back))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Fill);
		BackSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UHorizontalBox* Body = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AnswerBody"));
	if (UOverlaySlot* BodySlot = Layer->AddChildToOverlay(Body))
	{
		BodySlot->SetHorizontalAlignment(HAlign_Center);
		BodySlot->SetVerticalAlignment(VAlign_Center);
		BodySlot->SetPadding(FMargin(0.f, 140.f, 0.f, 10.f));
	}

	// Left: the sketchbook and the tools under it.
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AnswerLeft"));
	Body->AddChildToHorizontalBox(Left);

	UBorder* PadFace = nullptr;
	UOverlay* PadSticker = JinzzaUI::MakeSticker(WidgetTree, TEXT("PadFrame"), JinzzaUI::Sticker_Ink, 22.f, PadFace);
	PadFace->SetPadding(FMargin(10.f));
	Left->AddChildToVerticalBox(PadSticker);

	UOverlay* PadLayers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PadLayers"));
	PadFace->SetContent(PadLayers);
	USizeBox* PadSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PadSize"));
	PadSize->SetWidthOverride(JinzzaSketch::CanvasSize.X);
	PadSize->SetHeightOverride(JinzzaSketch::CanvasSize.Y);
	Pad = WidgetTree->ConstructWidget<UjinzzaSketchPad>(UjinzzaSketchPad::StaticClass(), TEXT("Pad"));
	PadSize->AddChild(Pad);
	PadLayers->AddChildToOverlay(PadSize);

	// "SUBMITTED!" / "TIME'S UP!" stamp slapped over the page.
	UBorder* StampFace = nullptr;
	UOverlay* StampSticker = JinzzaUI::MakeSticker(WidgetTree, TEXT("Stamp"), JinzzaUI::Sticker_Coral, 18.f, StampFace);
	StampFace->SetPadding(FMargin(30.f, 12.f));
	StampText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("StampText"), FText::GetEmpty(), 52);
	StampFace->SetContent(StampText);
	StampSticker->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	StampSticker->SetRenderTransformAngle(-10.f);
	StampSticker->SetVisibility(ESlateVisibility::Collapsed);
	Stamp = StampSticker;
	if (UOverlaySlot* StampSlot = PadLayers->AddChildToOverlay(StampSticker))
	{
		StampSlot->SetHorizontalAlignment(HAlign_Center);
		StampSlot->SetVerticalAlignment(VAlign_Center);
	}

	UHorizontalBox* Tools = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Tools"));
	if (UVerticalBoxSlot* ToolsSlot = JinzzaUI::AddSpaced(Left, Tools, 14.f))
	{
		ToolsSlot->SetHorizontalAlignment(HAlign_Center);
	}
	// Eraser first (palette index 0), then the markers.
	SwatchButtons.Add(MakeTool(Tools, FText::FromString(TEXT("Eraser")), JinzzaUI::Sticker_Sky, EJinzzaQuestionTool::Color, 0));
	const int32 PaletteNum = JinzzaSketch::GetPalette().Num();
	for (int32 ColorIndex = 1; ColorIndex < PaletteNum; ++ColorIndex)
	{
		SwatchButtons.Add(MakeSwatch(Tools, ColorIndex));
	}
	if (UHorizontalBoxSlot* GapSlot = Tools->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass())))
	{
		GapSlot->SetPadding(FMargin(10.f, 0.f));
	}
	const TCHAR* SizeLabels[] = { TEXT("Thin"), TEXT("Medium"), TEXT("Thick") };
	for (int32 SizeIndex = 0; SizeIndex < JinzzaSketch::NumBrushSizes; ++SizeIndex)
	{
		SizeButtons.Add(MakeTool(Tools, FText::FromString(SizeLabels[SizeIndex]), JinzzaUI::Sticker_Teal, EJinzzaQuestionTool::Size, SizeIndex));
	}
	MakeTool(Tools, FText::FromString(TEXT("Undo")), JinzzaUI::Sticker_Pink, EJinzzaQuestionTool::Undo, 0);
	MakeTool(Tools, FText::FromString(TEXT("Clear")), JinzzaUI::Sticker_Coral, EJinzzaQuestionTool::Clear, 0);

	// Right: clock, who's answered, ink left, hand-in button.
	USizeBox* RightWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AnswerRightWidth"));
	RightWidth->SetWidthOverride(300.f);
	if (UHorizontalBoxSlot* RightSlot = Body->AddChildToHorizontalBox(RightWidth))
	{
		RightSlot->SetPadding(FMargin(28.f, 0.f, 0.f, 0.f));
		RightSlot->SetVerticalAlignment(VAlign_Center);
	}
	UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AnswerRight"));
	RightWidth->AddChild(Right);

	UBorder* ClockFace = nullptr;
	UOverlay* ClockSticker = JinzzaUI::MakeSticker(WidgetTree, TEXT("Clock"), JinzzaUI::Sticker_Ink, 28.f, ClockFace);
	ClockFace->SetPadding(FMargin(20.f, 4.f));
	AnswerClock = JinzzaUI::MakeStickerText(WidgetTree, TEXT("AnswerClock"), FText::GetEmpty(), 84);
	AnswerClock->SetJustification(ETextJustify::Center);
	AnswerClock->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	ClockFace->SetContent(AnswerClock);
	Right->AddChildToVerticalBox(ClockSticker);

	AnsweredText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("AnsweredText"), FText::GetEmpty(), 22);
	JinzzaUI::AddSpaced(Right, AnsweredText, 22.f);
	AnsweredDots = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AnsweredDots"));
	JinzzaUI::AddSpaced(Right, AnsweredDots, 8.f);

	JinzzaUI::AddSpaced(Right, JinzzaUI::MakeStickerText(WidgetTree, TEXT("InkLabel"), FText::FromString(TEXT("Ink")), 18, true), 22.f);
	USizeBox* InkSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InkSize"));
	InkSize->SetHeightOverride(16.f);
	InkBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("InkBar"));
	InkBar->SetFillColorAndOpacity(JinzzaUI::Sticker_Teal);
	InkSize->AddChild(InkBar);
	JinzzaUI::AddSpaced(Right, InkSize, 6.f);

	UHorizontalBox* SubmitRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SubmitRow"));
	JinzzaUI::AddSpaced(Right, SubmitRow, 28.f);
	SubmitButton = MakeTool(SubmitRow, FText::FromString(TEXT("Hand it in!")), JinzzaUI::Sticker_Yellow, EJinzzaQuestionTool::SubmitDrawing, 0, 28.f);
	JinzzaUI::SetStickerButtonSelected(SubmitButton, JinzzaUI::Sticker_Yellow, true);

	AnswerHint = JinzzaUI::MakeStickerText(WidgetTree, TEXT("AnswerHint"), FText::GetEmpty(), 17, true);
	AnswerHint->SetAutoWrapText(true);
	JinzzaUI::AddSpaced(Right, AnswerHint, 14.f);

	return Layer;
}

UButton* UjinzzaQuestionWidget::MakeTool(UHorizontalBox* Row, const FText& Label, const FLinearColor& Accent, EJinzzaQuestionTool Tool, int32 Index, float FontSize)
{
	UButton* Button = JinzzaUI::MakeStickerButton(WidgetTree, NextName(TEXT("QTool")), Label, Accent, FontSize);
	if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button))
	{
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
		ButtonSlot->SetPadding(FMargin(4.f, 0.f));
	}

	UJinzzaQuestionToolHandler* Handler = NewObject<UJinzzaQuestionToolHandler>(this);
	Handler->OwnerWidget = this;
	Handler->Tool = Tool;
	Handler->Index = Index;
	Button->OnClicked.AddDynamic(Handler, &UJinzzaQuestionToolHandler::HandleClicked);
	Handlers.Add(Handler);
	return Button;
}

UButton* UjinzzaQuestionWidget::MakeSwatch(UHorizontalBox* Row, int32 ColorIndex)
{
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Size->SetWidthOverride(46.f);
	Size->SetHeightOverride(46.f);
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetStyle(MakeSwatchStyle(JinzzaSketch::GetPalette()[ColorIndex], false));
	Size->AddChild(Button);
	if (UHorizontalBoxSlot* SwatchSlot = Row->AddChildToHorizontalBox(Size))
	{
		SwatchSlot->SetVerticalAlignment(VAlign_Center);
		SwatchSlot->SetPadding(FMargin(4.f, 0.f));
	}

	UJinzzaQuestionToolHandler* Handler = NewObject<UJinzzaQuestionToolHandler>(this);
	Handler->OwnerWidget = this;
	Handler->Tool = EJinzzaQuestionTool::Color;
	Handler->Index = ColorIndex;
	Button->OnClicked.AddDynamic(Handler, &UJinzzaQuestionToolHandler::HandleClicked);
	Handlers.Add(Handler);
	return Button;
}

void UjinzzaQuestionWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
	SetVisibility(ESlateVisibility::Collapsed);
	RefreshToolHighlights();
}

void UjinzzaQuestionWidget::NativeDestruct()
{
	DestroyPanels();
	Super::NativeDestruct();
}

// --- Tools ----------------------------------------------------------------------------------------

void UjinzzaQuestionWidget::HandleTool(EJinzzaQuestionTool Tool, int32 Index)
{
	switch (Tool)
	{
	case EJinzzaQuestionTool::Color:
		Pad->SetColorIndex(Index);
		RefreshToolHighlights();
		break;
	case EJinzzaQuestionTool::Size:
		Pad->SetSizeIndex(Index);
		RefreshToolHighlights();
		break;
	case EJinzzaQuestionTool::Undo:
		if (!bSubmitted)
		{
			Pad->Undo();
		}
		break;
	case EJinzzaQuestionTool::Clear:
		if (!bSubmitted)
		{
			Pad->Clear();
		}
		break;
	case EJinzzaQuestionTool::SubmitDrawing:
		if (!bSubmitted && !Pad->GetDrawing().IsEmpty())
		{
			SubmitDrawing(false);
		}
		break;
	case EJinzzaQuestionTool::SubmitQuestion:
		SubmitQuestion();
		break;
	}
}

void UjinzzaQuestionWidget::RefreshToolHighlights()
{
	if (!Pad)
	{
		return;
	}
	for (int32 Index = 0; Index < SwatchButtons.Num(); ++Index)
	{
		UButton* Button = SwatchButtons[Index];
		if (!Button)
		{
			continue;
		}
		const bool bSelected = Pad->GetColorIndex() == Index;
		if (Index == 0)
		{
			JinzzaUI::SetStickerButtonSelected(Button, JinzzaUI::Sticker_Sky, bSelected);
		}
		else
		{
			Button->SetStyle(MakeSwatchStyle(JinzzaSketch::GetPalette()[Index], bSelected));
		}
	}
	for (int32 Index = 0; Index < SizeButtons.Num(); ++Index)
	{
		if (UButton* Button = SizeButtons[Index])
		{
			JinzzaUI::SetStickerButtonSelected(Button, JinzzaUI::Sticker_Teal, Pad->GetSizeIndex() == Index);
		}
	}
}

void UjinzzaQuestionWidget::SubmitDrawing(bool bTimeUp)
{
	AjinzzaGamePlayerController* PC = Cast<AjinzzaGamePlayerController>(GetOwningPlayer());
	if (bSubmitted || !PC || !Pad)
	{
		return;
	}
	bSubmitted = true;
	PC->Server_SubmitDrawing(Pad->GetDrawing());
	Pad->SetEditable(false);

	const bool bBlank = Pad->GetDrawing().IsEmpty();
	StampText->SetText(FText::FromString(bTimeUp ? TEXT("TIME'S UP!") : TEXT("SUBMITTED!")));
	Stamp->SetVisibility(ESlateVisibility::HitTestInvisible);
	StampAge = 0.f;
	AnswerHint->SetText(FText::FromString(bTimeUp && bBlank
		? TEXT("Nothing drawn - your board will show a big \"?\".")
		: TEXT("Sent! Your board flips round once everyone's done.")));
	PlaySound(bTimeUp ? SoundTimeUp : SoundSubmitted);
}

void UjinzzaQuestionWidget::SubmitQuestion()
{
	AjinzzaGamePlayerController* PC = Cast<AjinzzaGamePlayerController>(GetOwningPlayer());
	const FString Text = QuestionBox ? QuestionBox->GetText().ToString().TrimStartAndEnd() : FString();
	if (bQuestionSent || !PC || Text.IsEmpty())
	{
		return;
	}
	bQuestionSent = true;
	PC->Server_SubmitQuestion(Text.Left(MaxQuestionChars));
}

void UjinzzaQuestionWidget::HandleQuestionCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		SubmitQuestion();
	}
}

void UjinzzaQuestionWidget::HandleQuestionChanged(const FText& Text)
{
	FString Value = Text.ToString();
	if (Value.Len() > MaxQuestionChars)
	{
		Value.LeftInline(MaxQuestionChars);
		QuestionBox->SetText(FText::FromString(Value));
	}
	AskCounter->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Value.Len(), MaxQuestionChars)));
}

FName UjinzzaQuestionWidget::NextName(const TCHAR* Base)
{
	return FName(*FString::Printf(TEXT("%s_%d"), Base, ++NameCounter));
}

void UjinzzaQuestionWidget::PlaySound(const TCHAR* Path) const
{
	if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, Path))
	{
		UGameplayStatics::PlaySound2D(GetOwningPlayer(), Sound);
	}
}

// --- Split screen ---------------------------------------------------------------------------------

void UjinzzaQuestionWidget::SyncPanels(const FJinzzaQuestionState& State)
{
	bool bSame = Panels.Num() == State.Seats.Num();
	for (int32 Index = 0; bSame && Index < Panels.Num(); ++Index)
	{
		bSame = Panels[Index].Player == State.Seats[Index].Player;
	}
	if (bSame)
	{
		return;
	}

	DestroyPanels();
	UWorld* World = GetWorld();
	if (!World || State.Seats.Num() == 0)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CaptureRig = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (CaptureRig)
	{
		USceneComponent* RigRoot = NewObject<USceneComponent>(CaptureRig, TEXT("Root"));
		CaptureRig->SetRootComponent(RigRoot);
		RigRoot->RegisterComponent();
	}

	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* Me = PC ? PC->PlayerState : nullptr;
	const int32 Count = State.Seats.Num();
	const int32 Columns = QuestionGridColumns(Count);
	const bool bSmall = Count > 6;

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FJinzzaQuestionSeat& Seat = State.Seats[Index];
		FJinzzaQuestionPanel& Panel = Panels.AddDefaulted_GetRef();
		Panel.Player = Seat.Player;

		// Camera feed.
		Panel.Target = NewObject<UTextureRenderTarget2D>(this);
		Panel.Target->ClearColor = FLinearColor(0.02f, 0.02f, 0.03f, 1.f);
		Panel.Target->InitAutoFormat(bSmall ? PanelCaptureWidthSmall : PanelCaptureWidth, bSmall ? PanelCaptureHeightSmall : PanelCaptureHeight);
		Panel.Target->UpdateResourceImmediate(true);

		if (CaptureRig)
		{
			USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(CaptureRig);
			Capture->SetupAttachment(CaptureRig->GetRootComponent());
			Capture->SetUsingAbsoluteLocation(true);
			Capture->SetUsingAbsoluteRotation(true);
			Capture->FOVAngle = PanelCameraFOV;
			Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
			Capture->bCaptureEveryFrame = false;
			Capture->bCaptureOnMovement = false;
			Capture->bAlwaysPersistRenderingState = true;
			Capture->TextureTarget = Panel.Target;
			Capture->RegisterComponent();
			Capture->SetWorldLocationAndRotation(Seat.CameraLocation, Seat.CameraRotation);
			Panel.Capture = Capture;
		}

		// Panel: the feed (cropped to fill), a frame, the name tag and a status chip.
		UOverlay* Cell = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Cell->SetClipping(EWidgetClipping::ClipToBounds);

		UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Fit->SetStretch(EStretch::ScaleToFill);
		UImage* Feed = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Feed->SetBrushResourceObject(Panel.Target);
		Feed->SetDesiredSizeOverride(FVector2D(Panel.Target->SizeX, Panel.Target->SizeY));
		Fit->AddChild(Feed);
		if (UOverlaySlot* FeedSlot = Cell->AddChildToOverlay(Fit))
		{
			FeedSlot->SetHorizontalAlignment(HAlign_Fill);
			FeedSlot->SetVerticalAlignment(VAlign_Fill);
		}

		const bool bMine = Seat.Player && Seat.Player == Me;
		const FLinearColor FrameColor = bMine ? JinzzaUI::Sticker_Yellow : !Seat.bAnswerer ? JinzzaUI::Sticker_Sky : JinzzaUI::Sticker_White;
		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Frame->SetBrush(FSlateRoundedBoxBrush(FLinearColor::Transparent, 16.f, FrameColor, bMine ? 6.f : 4.f));
		Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* FrameSlot = Cell->AddChildToOverlay(Frame))
		{
			FrameSlot->SetHorizontalAlignment(HAlign_Fill);
			FrameSlot->SetVerticalAlignment(VAlign_Fill);
		}

		const FString Name = AjinzzaPartyPlayerState::GetDisplayNameFor(Seat.Player);
		UBorder* NameFace = nullptr;
		UOverlay* NameTag = JinzzaUI::MakeSticker(WidgetTree, NextName(TEXT("QNameTag")), bMine ? JinzzaUI::Sticker_Yellow : JinzzaUI::Sticker_Ink, 14.f, NameFace, 3.f, 4.f);
		NameFace->SetPadding(FMargin(14.f, 4.f));
		UTextBlock* NameText = JinzzaUI::MakeStickerText(WidgetTree, NextName(TEXT("QNameText")),
			FText::FromString(bMine ? FString::Printf(TEXT("You - %s"), *Name) : Name), 20);
		if (bMine)
		{
			NameText->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_Ink));
		}
		NameFace->SetContent(NameText);
		if (UOverlaySlot* NameSlot = Cell->AddChildToOverlay(NameTag))
		{
			NameSlot->SetHorizontalAlignment(HAlign_Left);
			NameSlot->SetVerticalAlignment(VAlign_Top);
			NameSlot->SetPadding(FMargin(12.f, 10.f));
		}

		UBorder* ChipFace = nullptr;
		UOverlay* Chip = JinzzaUI::MakeSticker(WidgetTree, NextName(TEXT("QChip")), JinzzaUI::Sticker_Coral, 14.f, ChipFace, 3.f, 4.f);
		Panel.StatusFace = ChipFace;
		Panel.StatusFace->SetPadding(FMargin(12.f, 4.f));
		Panel.StatusText = JinzzaUI::MakeStickerText(WidgetTree, NextName(TEXT("QChipText")), FText::GetEmpty(), 18);
		Panel.StatusFace->SetContent(Panel.StatusText);
		Chip->SetVisibility(ESlateVisibility::Collapsed);
		Panel.StatusChip = Chip;
		if (UOverlaySlot* ChipSlot = Cell->AddChildToOverlay(Chip))
		{
			ChipSlot->SetHorizontalAlignment(HAlign_Right);
			ChipSlot->SetVerticalAlignment(VAlign_Top);
			ChipSlot->SetPadding(FMargin(12.f, 10.f));
		}

		if (UUniformGridSlot* CellSlot = Grid->AddChildToUniformGrid(Cell, Index / Columns, Index % Columns))
		{
			CellSlot->SetHorizontalAlignment(HAlign_Fill);
			CellSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
	bCapturesActive = false;
}

void UjinzzaQuestionWidget::DestroyPanels()
{
	if (Grid)
	{
		Grid->ClearChildren();
	}
	for (FJinzzaQuestionPanel& Panel : Panels)
	{
		if (Panel.Target)
		{
			Panel.Target->ReleaseResource();
		}
	}
	Panels.Reset();
	if (CaptureRig)
	{
		CaptureRig->Destroy();
		CaptureRig = nullptr;
	}
	bCapturesActive = false;
}

void UjinzzaQuestionWidget::SetCapturesActive(bool bActive)
{
	if (bActive == bCapturesActive)
	{
		return;
	}
	bCapturesActive = bActive;
	// Only render the cameras while the split screen is actually on screen.
	for (FJinzzaQuestionPanel& Panel : Panels)
	{
		if (Panel.Capture)
		{
			Panel.Capture->bCaptureEveryFrame = bActive;
		}
	}
}

void UjinzzaQuestionWidget::UpdatePanels(const FJinzzaQuestionState& State)
{
	for (FJinzzaQuestionPanel& Panel : Panels)
	{
		const FJinzzaQuestionSeat* Seat = State.FindSeat(Panel.Player);
		FString Status;
		FLinearColor ChipColor = JinzzaUI::Sticker_Coral;
		if (Seat && State.Step == EJinzzaQuestionStep::Asking && !Seat->bAnswerer)
		{
			Status = TEXT("Asking...");
			ChipColor = JinzzaUI::Sticker_Sky;
		}
		else if (Seat && State.Step == EJinzzaQuestionStep::Answering && Seat->bAnswerer)
		{
			const bool bDone = State.HasSubmitted(Panel.Player);
			Status = bDone ? TEXT("Done!") : TEXT("Drawing...");
			ChipColor = bDone ? JinzzaUI::Sticker_Teal : JinzzaUI::Sticker_Coral;
		}

		if (Panel.StatusChip)
		{
			Panel.StatusChip->SetVisibility(Status.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
			Panel.StatusText->SetText(FText::FromString(Status));
			Panel.StatusFace->SetBrushColor(ChipColor);
		}

		// Pop as each answer board flips round.
		const APawn* Pawn = Panel.Player ? Panel.Player->GetPawn() : nullptr;
		const UjinzzaChatBoardComponent* Board = Pawn ? Pawn->FindComponentByClass<UjinzzaChatBoardComponent>() : nullptr;
		const bool bShowing = Board && Board->GetBoardState() == EJinzzaChatBoardState::Showing && State.Step == EJinzzaQuestionStep::Showing;
		if (bShowing && !Panel.bBoardShowing && bCapturesActive)
		{
			PlaySound(SoundBoardFlip);
		}
		Panel.bBoardShowing = bShowing;
	}
}

// --- Per frame ------------------------------------------------------------------------------------

void UjinzzaQuestionWidget::OnStepEntered(const FJinzzaQuestionState& State, bool bAsker, bool bAnswerer)
{
	APlayerController* PC = GetOwningPlayer();

	switch (State.Step)
	{
	case EJinzzaQuestionStep::Asking:
		bQuestionSent = false;
		if (bAsker && QuestionBox && PC)
		{
			QuestionBox->SetText(FText::GetEmpty());
			HandleQuestionChanged(FText::GetEmpty());
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(QuestionBox->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
		break;

	case EJinzzaQuestionStep::Revealing:
		PlaySound(SoundSignDrop);
		break;

	case EJinzzaQuestionStep::Answering:
		// New answers are coming - drop last cycle's.
		if (const AjinzzaGameGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AjinzzaGameGameState>() : nullptr)
		{
			for (APlayerState* PS : MatchState->PlayerArray)
			{
				if (AjinzzaPartyPlayerState* PartyPS = Cast<AjinzzaPartyPlayerState>(PS))
				{
					PartyPS->ClearLocalRevealedDrawing();
				}
			}
		}
		bSubmitted = false;
		LastTickSecond = -1;
		ShownAnsweredCount = -1;
		StampAge = -1.f;
		if (Stamp)
		{
			Stamp->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Pad)
		{
			Pad->SetDrawing(FJinzzaDrawing());
			Pad->SetEditable(bAnswerer);
			Pad->SetColorIndex(1);
			Pad->SetSizeIndex(1);
			RefreshToolHighlights();
		}
		if (AnswerHint)
		{
			AnswerHint->SetText(FText::FromString(TEXT("Draw your answer - hold the left mouse button. Hand it in when you're happy; whatever is on the page when time runs out goes in.")));
		}
		break;

	default:
		break;
	}
}

void UjinzzaQuestionWidget::UpdateSign(const FJinzzaQuestionState& State, double ServerNow)
{
	const bool bSignUp = !State.Question.IsEmpty() &&
		(State.Step == EJinzzaQuestionStep::Revealing || State.Step == EJinzzaQuestionStep::Answering || State.Step == EJinzzaQuestionStep::Showing);
	Sign->SetVisibility(bSignUp ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bSignUp)
	{
		return;
	}

	const FText Question = FText::FromString(State.Question);
	if (!SignText->GetText().EqualTo(Question))
	{
		SignText->SetText(Question);
	}

	// Drops in from above with a bounce, swings on its strings and settles; big while it's being read out.
	const float T = FMath::Max(0.f, static_cast<float>(ServerNow - State.RevealServerTime));
	const float Drop = SignDropDistance * (1.f - QuestionBounceOut(FMath::Clamp(T / SignDropSeconds, 0.f, 1.f)));
	const float Swing = 7.f * FMath::Sin(T * 8.f) * FMath::Exp(-2.2f * T);
	const float ShrinkStart = 2.3f;
	const float ShrinkEnd = 3.f;
	const float Scale = FMath::Lerp(SignRevealScale, 1.f, FMath::Clamp((T - ShrinkStart) / (ShrinkEnd - ShrinkStart), 0.f, 1.f));
	Sign->SetRenderTranslation(FVector2D(0.f, -Drop));
	Sign->SetRenderTransformAngle(Swing);
	Sign->SetRenderScale(FVector2D(Scale, Scale));
}

void UjinzzaQuestionWidget::UpdateAnswerScreen(const FJinzzaQuestionState& State, float Remaining)
{
	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* Me = PC ? PC->PlayerState : nullptr;

	// Server already has it (e.g. this widget was rebuilt) - treat as handed in.
	if (!bSubmitted && State.HasSubmitted(Me))
	{
		bSubmitted = true;
		Pad->SetEditable(false);
	}
	// Hand in whatever is there just before the deadline (the server allows a short grace for the trip).
	if (!bSubmitted && Remaining <= 0.5f)
	{
		SubmitDrawing(true);
	}

	const int32 Seconds = FMath::CeilToInt(Remaining);
	const bool bHurry = Seconds <= 5;
	AnswerClock->SetText(FText::AsNumber(Seconds));
	AnswerClock->SetColorAndOpacity(FSlateColor(bHurry ? JinzzaUI::Sticker_Coral : JinzzaUI::Sticker_White));
	const float Pulse = bHurry ? 1.f + 0.15f * FMath::Abs(FMath::Sin(Remaining * PI)) : 1.f;
	AnswerClock->SetRenderScale(FVector2D(Pulse, Pulse));

	// Who has answered: one dot per answerer, filled once they're in.
	const int32 Answered = State.Submitted.Num();
	const int32 Total = State.CountAnswerers();
	if (Answered != ShownAnsweredCount || AnsweredDots->GetChildrenCount() != Total)
	{
		if (ShownAnsweredCount >= 0 && Answered > ShownAnsweredCount)
		{
			PlaySound(SoundTick);
		}
		ShownAnsweredCount = Answered;
		AnsweredText->SetText(FText::FromString(FString::Printf(TEXT("Answered %d / %d"), Answered, Total)));
		AnsweredDots->ClearChildren();
		for (int32 Index = 0; Index < Total; ++Index)
		{
			UHorizontalBoxSlot* DotSlot = AnsweredDots->AddChildToHorizontalBox(
				JinzzaUI::MakeStickerDot(WidgetTree, NextName(TEXT("QDot")), Index < Answered ? JinzzaUI::Sticker_Teal : JinzzaUI::Sticker_Ink, 18.f));
			if (DotSlot)
			{
				DotSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
			}
		}
	}

	const float Ink = Pad->GetInkUsed();
	InkBar->SetPercent(1.f - Ink);
	InkBar->SetFillColorAndOpacity(Ink > 0.85f ? JinzzaUI::Sticker_Coral : JinzzaUI::Sticker_Teal);

	SubmitButton->SetIsEnabled(!bSubmitted && !Pad->GetDrawing().IsEmpty());

	if (StampAge >= 0.f)
	{
		// Slapped on: big and fast, then settles.
		StampAge += FApp::GetDeltaTime();
		const float Pop = FMath::Lerp(1.8f, 1.f, FMath::Clamp(StampAge / 0.18f, 0.f, 1.f));
		Stamp->SetRenderScale(FVector2D(Pop, Pop));
	}
}

void UjinzzaQuestionWidget::Refresh(float DeltaTime)
{
	const AjinzzaGameGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AjinzzaGameGameState>() : nullptr;
	if (!MatchState || !Grid)
	{
		return;
	}

	const FJinzzaQuestionState& State = MatchState->GetQuestionState();
	APlayerController* PC = GetOwningPlayer();
	const APlayerState* Me = PC ? PC->PlayerState : nullptr;

	if (!State.IsActive())
	{
		bWantsCursor = false;
		if (ShownStep != EJinzzaQuestionStep::None)
		{
			DestroyPanels();
			ShownStep = EJinzzaQuestionStep::None;
			ShownSerial = State.Serial;
			SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	const FJinzzaQuestionSeat* MySeat = State.FindSeat(Me);
	const bool bAsker = MySeat && !MySeat->bAnswerer;
	const bool bAnswerer = MySeat && MySeat->bAnswerer;

	SyncPanels(State);
	if (State.Serial != ShownSerial)
	{
		ShownSerial = State.Serial;
		ShownStep = State.Step;
		OnStepEntered(State, bAsker, bAnswerer);
	}

	const float Remaining = MatchState->GetQuestionStepTimeRemaining();
	const int32 Seconds = FMath::CeilToInt(Remaining);
	const bool bAnswerScreen = State.Step == EJinzzaQuestionStep::Answering && bAnswerer;
	bWantsCursor = (State.Step == EJinzzaQuestionStep::Asking && bAsker && !bQuestionSent) || (bAnswerScreen && !bSubmitted);

	// One person's drawing screen while they answer; the split screen whenever people interact.
	SplitLayer->SetVisibility(bAnswerScreen ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	AnswerLayer->SetVisibility(bAnswerScreen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	AskLayer->SetVisibility(State.Step == EJinzzaQuestionStep::Asking && bAsker && !bQuestionSent
		? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	SetCapturesActive(!bAnswerScreen);

	// Banner while the question is being written.
	if (State.Step == EJinzzaQuestionStep::Asking)
	{
		const FString Message = bAsker
			? (bQuestionSent
				? FString::Printf(TEXT("Question %d/%d - sent!"), State.Cycle, State.TotalCycles)
				: FString::Printf(TEXT("Question %d/%d - type your question - %ds"), State.Cycle, State.TotalCycles, Seconds))
			: FString::Printf(TEXT("Question %d/%d - the Judge is writing a question. Talk it over! %ds"), State.Cycle, State.TotalCycles, Seconds);
		BannerText->SetText(FText::FromString(Message));
		Banner->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Banner->SetVisibility(ESlateVisibility::Collapsed);
	}

	UpdateSign(State, MatchState->GetServerWorldTimeSeconds());

	// Countdown under the sign: answer time (red + ticking at the end), then discussion time.
	const bool bShowCountdown = State.Step == EJinzzaQuestionStep::Answering || State.Step == EJinzzaQuestionStep::Showing;
	Countdown->SetVisibility(bShowCountdown && !bAnswerScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bShowCountdown)
	{
		const bool bAnswering = State.Step == EJinzzaQuestionStep::Answering;
		const bool bHurry = bAnswering && Seconds <= 5;
		CountdownText->SetText(FText::FromString(bAnswering
			? FString::Printf(TEXT("Answer time  %d"), Seconds)
			: FString::Printf(TEXT("Discuss!  %d"), Seconds)));
		CountdownFace->SetBrushColor(bHurry ? JinzzaUI::Sticker_Coral : JinzzaUI::Sticker_Ink);
		const float Pulse = bHurry ? 1.f + 0.12f * FMath::Abs(FMath::Sin(Remaining * PI)) : 1.f;
		Countdown->SetRenderScale(FVector2D(Pulse, Pulse));

		if (bHurry && Seconds > 0 && Seconds != LastTickSecond)
		{
			LastTickSecond = Seconds;
			PlaySound(SoundTick);
		}
	}

	UpdatePanels(State);
	if (bAnswerScreen)
	{
		UpdateAnswerScreen(State, Remaining);
	}
}
