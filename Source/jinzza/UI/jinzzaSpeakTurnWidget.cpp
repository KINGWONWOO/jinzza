// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaSpeakTurnWidget.h"
#include "jinzzaGameGameState.h"
#include "jinzzaGamePlayerController.h"
#include "jinzzaPartyPlayerState.h"
#include "jinzzaUIStyle.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/World.h"

namespace
{
	constexpr int32 MaxBubblesPerSide = 5;
	constexpr float BubbleMaxWidth = 440.f;
}

void UjinzzaSpeakTurnWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Top-center banner: black sticker pill.
	UBorder* BannerFace = nullptr;
	UOverlay* BannerSticker = JinzzaUI::MakeSticker(WidgetTree, TEXT("Banner"), JinzzaUI::Sticker_Ink, 24.f, BannerFace);
	BannerFace->SetPadding(FMargin(26.f, 10.f));
	BannerText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("BannerText"), FText::GetEmpty(), 24);
	BannerText->SetJustification(ETextJustify::Center);
	BannerFace->SetContent(BannerText);
	Banner = BannerSticker;
	if (UOverlaySlot* BannerSlot = Root->AddChildToOverlay(BannerSticker))
	{
		BannerSlot->SetHorizontalAlignment(HAlign_Center);
		BannerSlot->SetVerticalAlignment(VAlign_Top);
		BannerSlot->SetPadding(FMargin(0.f, 28.f, 0.f, 0.f));
	}

	// Bubble columns on the left and right edges, vertically centered.
	auto MakeColumn = [&](FName Name, EHorizontalAlignment Side) -> UVerticalBox*
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
		if (UOverlaySlot* ColumnSlot = Root->AddChildToOverlay(Column))
		{
			ColumnSlot->SetHorizontalAlignment(Side);
			ColumnSlot->SetVerticalAlignment(VAlign_Center);
			ColumnSlot->SetPadding(FMargin(40.f, 0.f));
		}
		return Column;
	};
	LeftBubbles = MakeColumn(TEXT("LeftBubbles"), HAlign_Left);
	RightBubbles = MakeColumn(TEXT("RightBubbles"), HAlign_Right);
}

void UjinzzaSpeakTurnWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (Banner)
	{
		Banner->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UWidget* UjinzzaSpeakTurnWidget::MakeBubble(const FString& SpeakerName, const FString& Text, bool bOwn)
{
	// Rounded bubble: white for the speaker as others see them, yellow for your own lines.
	UVerticalBox* Bubble = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	if (!bOwn)
	{
		UTextBlock* Name = JinzzaUI::MakeStickerText(WidgetTree, NAME_None, FText::FromString(SpeakerName), 16);
		Name->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Name->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		if (UVerticalBoxSlot* NameSlot = Bubble->AddChildToVerticalBox(Name))
		{
			NameSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 4.f));
		}
	}

	UBorder* Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Body->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 18.f, JinzzaUI::Sticker_Ink, 3.f));
	Body->SetBrushColor(bOwn ? JinzzaUI::Sticker_Yellow : JinzzaUI::Sticker_White);
	Body->SetPadding(FMargin(16.f, 10.f));

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Width->SetMaxDesiredWidth(BubbleMaxWidth);
	UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Line->SetText(FText::FromString(Text));
	Line->SetFont(JinzzaUI::BodyFont(20));
	Line->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_Ink));
	Line->SetAutoWrapText(true);
	Width->AddChild(Line);
	Body->SetContent(Width);

	if (UVerticalBoxSlot* BodySlot = Bubble->AddChildToVerticalBox(Body))
	{
		BodySlot->SetHorizontalAlignment(bOwn ? HAlign_Right : HAlign_Left);
	}
	return Bubble;
}

void UjinzzaSpeakTurnWidget::AddBubble(const FString& SpeakerName, const FString& Text, bool bOwn)
{
	UVerticalBox* Column = bOwn ? RightBubbles : LeftBubbles;
	if (!Column || !WidgetTree)
	{
		return;
	}

	if (UVerticalBoxSlot* BubbleSlot = Column->AddChildToVerticalBox(MakeBubble(SpeakerName, Text, bOwn)))
	{
		BubbleSlot->SetHorizontalAlignment(bOwn ? HAlign_Right : HAlign_Left);
		BubbleSlot->SetPadding(FMargin(0.f, 6.f));
	}
	while (Column->GetChildrenCount() > MaxBubblesPerSide)
	{
		Column->RemoveChildAt(0);
	}
}

void UjinzzaSpeakTurnWidget::ClearBubbles()
{
	if (LeftBubbles)
	{
		LeftBubbles->ClearChildren();
	}
	if (RightBubbles)
	{
		RightBubbles->ClearChildren();
	}
}

void UjinzzaSpeakTurnWidget::Refresh()
{
	const AjinzzaGameGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AjinzzaGameGameState>() : nullptr;
	if (!MatchState || !Banner || !BannerText)
	{
		return;
	}

	const FJinzzaSpeakTurn& Turn = MatchState->GetSpeakTurn();
	if (Turn.Serial != ShownTurnSerial)
	{
		// New turn (or turn over): bubbles belong to one turn only.
		ShownTurnSerial = Turn.Serial;
		ClearBubbles();
	}

	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* Me = PC ? PC->PlayerState : nullptr;

	FString Message;
	if (MatchState->IsSpeakTurnActive())
	{
		const int32 Seconds = FMath::CeilToInt(MatchState->GetSpeakTurnTimeRemaining());
		const bool bMine = MatchState->IsTurnSpeaker(Me);
		const FString SpeakerName = AjinzzaPartyPlayerState::GetDisplayNameFor(Turn.Speaker);
		if (Turn.Kind == EJinzzaSpeakTurnKind::SelfIntroduction)
		{
			Message = bMine
				? FString::Printf(TEXT("Your turn! Introduce yourself (%d/%d) - %ds"), Turn.Number, Turn.Total, Seconds)
				: FString::Printf(TEXT("Self-introduction %d/%d - %s is speaking - %ds"), Turn.Number, Turn.Total, *SpeakerName, Seconds);
		}
		else
		{
			Message = bMine
				? FString::Printf(TEXT("You got the most votes - final argument - %ds"), Seconds)
				: FString::Printf(TEXT("%s got the most votes - final argument - %ds"), *SpeakerName, Seconds);
		}
	}
	else if (MatchState->IsVoteOpen())
	{
		const int32 Seconds = FMath::CeilToInt(MatchState->GetVoteTimeRemaining());
		const AjinzzaGamePlayerController* GamePC = Cast<AjinzzaGamePlayerController>(PC);
		const bool bJudge = GamePC && GamePC->GetLocalPartyRole() == EJinzzaPartyRole::Judge;
		const bool bFinal = MatchState->GetVotePhase() == EJinzzaRoundPhase::FinalDecision;
		Message = bJudge
			? FString::Printf(TEXT("%s - pick a candidate - %ds"), bFinal ? TEXT("Final decision") : TEXT("Evaluation"), Seconds)
			: FString::Printf(TEXT("The Judge is voting... %ds"), Seconds);
	}

	if (Message.IsEmpty())
	{
		Banner->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		BannerText->SetText(FText::FromString(Message));
		Banner->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}
