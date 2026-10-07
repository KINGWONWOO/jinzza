// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaVoteWidget.h"
#include "jinzzaGameGameState.h"
#include "jinzzaGamePlayerController.h"
#include "jinzzaPartyPlayerState.h"
#include "jinzzaUIStyle.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/World.h"

void UJinzzaVoteButtonHandler::HandleClicked()
{
	if (UjinzzaVoteWidget* Owner = OwnerWidget.Get())
	{
		Owner->Vote(Candidate.Get());
	}
}

void UjinzzaVoteWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Face = nullptr;
	UOverlay* PanelSticker = JinzzaUI::MakeStickerPanel(WidgetTree, TEXT("Panel"), Face);
	Panel = PanelSticker;
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelSticker))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Right);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
		PanelSlot->SetPadding(FMargin(0.f, 0.f, 48.f, 0.f));
	}

	USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelWidth"));
	Width->SetWidthOverride(320.f);
	Face->SetContent(Width);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));
	Width->AddChild(Stack);
	Stack->AddChildToVerticalBox(JinzzaUI::MakeStickerHeading(WidgetTree, TEXT("Heading"), FText::FromString(TEXT("Your vote")), 30));
	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeStickerText(WidgetTree, TEXT("Hint"),
		FText::FromString(TEXT("They give a final argument, then they're out.")), 16, true), 4.f);

	ButtonList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ButtonList"));
	JinzzaUI::AddSpaced(Stack, ButtonList, 14.f);
}

void UjinzzaVoteWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (Panel)
	{
		Panel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UjinzzaVoteWidget::RebuildButtons(const TArray<APlayerState*>& Candidates)
{
	ButtonList->ClearChildren();
	Handlers.Reset();
	ShownCandidates.Reset();
	CandidateButtons.Reset();

	for (APlayerState* Candidate : Candidates)
	{
		UButton* Button = JinzzaUI::MakeStickerButton(WidgetTree, NAME_None,
			FText::FromString(AjinzzaPartyPlayerState::GetDisplayNameFor(Candidate)), JinzzaUI::Sticker_Coral, 22.f);
		JinzzaUI::AddSpaced(ButtonList, Button, ShownCandidates.Num() == 0 ? 0.f : 8.f);

		UJinzzaVoteButtonHandler* Handler = NewObject<UJinzzaVoteButtonHandler>(this);
		Handler->OwnerWidget = this;
		Handler->Candidate = Candidate;
		Button->OnClicked.AddDynamic(Handler, &UJinzzaVoteButtonHandler::HandleClicked);
		Handlers.Add(Handler);

		ShownCandidates.Add(Candidate);
		CandidateButtons.Add(Button);
	}
}

void UjinzzaVoteWidget::Vote(APlayerState* Candidate)
{
	AjinzzaGamePlayerController* PC = Cast<AjinzzaGamePlayerController>(GetOwningPlayer());
	if (!PC || !Candidate)
	{
		return;
	}
	PC->Server_CastVote(Candidate);
	VotedFor = Candidate;

	for (int32 Index = 0; Index < CandidateButtons.Num(); ++Index)
	{
		if (UButton* Button = CandidateButtons[Index].Get())
		{
			JinzzaUI::SetStickerButtonSelected(Button, JinzzaUI::Sticker_Coral, ShownCandidates[Index] == Candidate);
		}
	}
}

void UjinzzaVoteWidget::Refresh()
{
	const AjinzzaGameGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AjinzzaGameGameState>() : nullptr;
	const AjinzzaGamePlayerController* PC = Cast<AjinzzaGamePlayerController>(GetOwningPlayer());
	const bool bOpen = MatchState && MatchState->IsVoteOpen() && PC && PC->GetLocalPartyRole() == EJinzzaPartyRole::Judge;

	if (!bOpen)
	{
		if (bWasOpen && Panel)
		{
			Panel->SetVisibility(ESlateVisibility::Collapsed);
		}
		bWasOpen = false;
		return;
	}

	// Living candidates, User1 first.
	TArray<APlayerState*> Candidates;
	for (APlayerState* PS : MatchState->PlayerArray)
	{
		const AjinzzaPartyPlayerState* PartyPS = Cast<AjinzzaPartyPlayerState>(PS);
		if (PartyPS && PartyPS->IsLivingCandidate() && PS != PC->PlayerState)
		{
			Candidates.Add(PS);
		}
	}
	Candidates.Sort([](const APlayerState& A, const APlayerState& B)
	{
		return CastChecked<AjinzzaPartyPlayerState>(&A)->GetAliasUserNumber() < CastChecked<AjinzzaPartyPlayerState>(&B)->GetAliasUserNumber();
	});

	bool bChanged = !bWasOpen || Candidates.Num() != ShownCandidates.Num();
	for (int32 Index = 0; !bChanged && Index < Candidates.Num(); ++Index)
	{
		bChanged = ShownCandidates[Index] != Candidates[Index];
	}
	if (bChanged)
	{
		if (!bWasOpen)
		{
			VotedFor.Reset();
		}
		RebuildButtons(Candidates);
	}

	if (!bWasOpen && Panel)
	{
		Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	bWasOpen = true;
}
