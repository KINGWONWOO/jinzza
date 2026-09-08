// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaEmoteWheelWidget.h"
#include "jinzzaUIStyle.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "GameFramework/PlayerController.h"

void UjinzzaEmoteWheelWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	auto AddQuadrantLabel = [this, Root](const TCHAR* Name, const FText& Text, EHorizontalAlignment HAlign, EVerticalAlignment VAlign, const FMargin& LabelPadding) -> UTextBlock*
	{
		UTextBlock* Label = JinzzaUI::MakeSectionHeading(WidgetTree, Name, Text);
		if (UOverlaySlot* Slot = Root->AddChildToOverlay(Label))
		{
			Slot->SetHorizontalAlignment(HAlign);
			Slot->SetVerticalAlignment(VAlign);
			Slot->SetPadding(LabelPadding);
		}
		return Label;
	};

	// Cross layout matching the Up/Down/Left/Right quadrants NativeTick already computes.
	ThumbsUpLabel = AddQuadrantLabel(TEXT("ThumbsUpLabel"), FText::FromString(TEXT("Thumbs Up")), HAlign_Center, VAlign_Top, FMargin(0.f, 100.f, 0.f, 0.f));
	ThumbsDownLabel = AddQuadrantLabel(TEXT("ThumbsDownLabel"), FText::FromString(TEXT("Thumbs Down")), HAlign_Center, VAlign_Bottom, FMargin(0.f, 0.f, 0.f, 100.f));
	MiddleFingerLabel = AddQuadrantLabel(TEXT("MiddleFingerLabel"), FText::FromString(TEXT("Middle Finger")), HAlign_Left, VAlign_Center, FMargin(120.f, 0.f, 0.f, 0.f));
	PointLabel = AddQuadrantLabel(TEXT("PointLabel"), FText::FromString(TEXT("Point")), HAlign_Right, VAlign_Center, FMargin(0.f, 0.f, 120.f, 0.f));

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
		if (Label)
		{
			Label->SetColorAndOpacity(FSlateColor(HoveredEmote == Emote ? JinzzaUI::Color_Accent : JinzzaUI::Color_TextMuted));
		}
	};

	ApplyStyle(ThumbsUpLabel, EJinzzaEmoteType::ThumbsUp);
	ApplyStyle(ThumbsDownLabel, EJinzzaEmoteType::ThumbsDown);
	ApplyStyle(MiddleFingerLabel, EJinzzaEmoteType::MiddleFinger);
	ApplyStyle(PointLabel, EJinzzaEmoteType::Point);
}
