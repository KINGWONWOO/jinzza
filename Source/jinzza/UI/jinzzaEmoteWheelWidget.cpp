// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaEmoteWheelWidget.h"
#include "jinzzaUIStyle.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/PlayerController.h"

void UjinzzaEmoteWheelWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Everything below lives inside one fixed-size, screen-centered square (not sized/positioned
	// relative to the viewport edges like the old cross layout was) so the backdrop circle, its
	// X-divider, and the four quadrant labels all stay proportional to each other and to the
	// mouse dead-zone/quadrant math in NativeTick regardless of resolution.
	constexpr float WheelDiameter = 380.f;

	USizeBox* WheelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WheelBox"));
	WheelBox->SetWidthOverride(WheelDiameter);
	WheelBox->SetHeightOverride(WheelDiameter);

	UOverlay* Wheel = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Wheel"));
	WheelBox->AddChild(Wheel);

	if (UOverlaySlot* WheelSlot = Root->AddChildToOverlay(WheelBox))
	{
		WheelSlot->SetHorizontalAlignment(HAlign_Center);
		WheelSlot->SetVerticalAlignment(VAlign_Center);
	}

	// The circle - fills the Wheel square, corner radius = half the size makes a Border a circle
	// (same FSlateRoundedBoxBrush trick as JinzzaUI::MakeCircleIconButton).
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrush(FSlateRoundedBoxBrush(FLinearColor(JinzzaUI::Color_Panel.R, JinzzaUI::Color_Panel.G, JinzzaUI::Color_Panel.B, 0.85f), WheelDiameter * 0.5f, JinzzaUI::Color_PanelBorder, 2.f));
	Wheel->AddChildToOverlay(Backdrop);

	// X-divider - NativeTick already splits the wheel into Up/Down/Left/Right by comparing
	// |mouse.x| vs |mouse.y| from center, which is exactly the two screen diagonals; drawing two
	// bars rotated +/-45 and crossing at center visualizes that same split as a literal "X".
	auto AddDividerBar = [this, Wheel, WheelDiameter](const TCHAR* Name, float AngleDegrees)
	{
		UWidget* Bar = JinzzaUI::MakeDivider(WidgetTree, Name, WheelDiameter * 0.9f, 3.f);
		Bar->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Bar->SetRenderTransformAngle(AngleDegrees);
		if (UOverlaySlot* BarSlot = Wheel->AddChildToOverlay(Bar))
		{
			BarSlot->SetHorizontalAlignment(HAlign_Center);
			BarSlot->SetVerticalAlignment(VAlign_Center);
		}
	};
	AddDividerBar(TEXT("DividerBar_NE_SW"), 45.f);
	AddDividerBar(TEXT("DividerBar_NW_SE"), -45.f);

	auto AddQuadrantLabel = [this, Wheel](const TCHAR* Name, const FText& Text, EHorizontalAlignment HAlign, EVerticalAlignment VAlign, const FMargin& LabelPadding) -> UTextBlock*
	{
		UTextBlock* Label = JinzzaUI::MakeSectionHeading(WidgetTree, Name, Text);
		Label->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		if (UOverlaySlot* Slot = Wheel->AddChildToOverlay(Label))
		{
			Slot->SetHorizontalAlignment(HAlign);
			Slot->SetVerticalAlignment(VAlign);
			Slot->SetPadding(LabelPadding);
		}
		return Label;
	};

	// Positioned inset from the Wheel square's own edges (not the viewport's), so each label
	// sits near the circle's rim inside the correct Up/Down/Left/Right quadrant.
	constexpr float LabelInset = 24.f;
	ThumbsUpLabel = AddQuadrantLabel(TEXT("ThumbsUpLabel"), FText::FromString(TEXT("Thumbs Up")), HAlign_Center, VAlign_Top, FMargin(0.f, LabelInset, 0.f, 0.f));
	ThumbsDownLabel = AddQuadrantLabel(TEXT("ThumbsDownLabel"), FText::FromString(TEXT("Thumbs Down")), HAlign_Center, VAlign_Bottom, FMargin(0.f, 0.f, 0.f, LabelInset));
	MiddleFingerLabel = AddQuadrantLabel(TEXT("MiddleFingerLabel"), FText::FromString(TEXT("Middle Finger")), HAlign_Left, VAlign_Center, FMargin(LabelInset, 0.f, 0.f, 0.f));
	PointLabel = AddQuadrantLabel(TEXT("PointLabel"), FText::FromString(TEXT("Point")), HAlign_Right, VAlign_Center, FMargin(0.f, 0.f, LabelInset, 0.f));

	RefreshHighlight();
}

void UjinzzaEmoteWheelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
}

void UjinzzaEmoteWheelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	float MouseX = 0.f, MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const FVector2D Center = ViewportSize * 0.5f;
	const FVector2D Delta = FVector2D(MouseX, MouseY) - Center;

	EJinzzaEmoteType NewHovered = EJinzzaEmoteType::None;

	// Dead zone at screen center so resting the mouse there doesn't commit to a direction.
	constexpr float DeadZoneSq = 20.f * 20.f;
	if (Delta.SizeSquared() >= DeadZoneSq)
	{
		if (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y))
		{
			NewHovered = Delta.X >= 0.f ? EJinzzaEmoteType::Point : EJinzzaEmoteType::MiddleFinger;
		}
		else
		{
			// Screen space Y grows downward, so a negative Y delta is "up".
			NewHovered = Delta.Y < 0.f ? EJinzzaEmoteType::ThumbsUp : EJinzzaEmoteType::ThumbsDown;
		}
	}

	if (NewHovered != HoveredEmote)
	{
		HoveredEmote = NewHovered;
		RefreshHighlight();
		BP_OnHoveredEmoteChanged(HoveredEmote);
	}
}

void UjinzzaEmoteWheelWidget::RefreshHighlight()
{
	auto ApplyStyle = [this](UTextBlock* Label, EJinzzaEmoteType Emote)
	{
		if (!Label)
		{
			return;
		}
		const bool bSelected = HoveredEmote == Emote;
		Label->SetColorAndOpacity(FSlateColor(bSelected ? JinzzaUI::Color_Accent : JinzzaUI::Color_TextMuted));
		// Enlarging the hovered slice is the wheel's only "you're about to pick this" cue for now
		// - there's no per-emote icon art yet (see class comment) for a nicer treatment.
		Label->SetRenderScale(FVector2D(bSelected ? 1.35f : 1.f));
	};

	ApplyStyle(ThumbsUpLabel, EJinzzaEmoteType::ThumbsUp);
	ApplyStyle(ThumbsDownLabel, EJinzzaEmoteType::ThumbsDown);
	ApplyStyle(MiddleFingerLabel, EJinzzaEmoteType::MiddleFinger);
	ApplyStyle(PointLabel, EJinzzaEmoteType::Point);
}
