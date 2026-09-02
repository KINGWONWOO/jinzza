// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyWidget.h"
#include "jinzzaUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaLobbyGameState.h"
#include "jinzzaGameInstance.h"

void UjinzzaLobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Panel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("Panel"));
	Panel->SetHorizontalAlignment(HAlign_Right);
	Panel->SetVerticalAlignment(VAlign_Top);
	Panel->SetPadding(FMargin(24.f));

	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
	PanelSlot->SetPosition(FVector2D(-24.f, 24.f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LobbyBox"));
	Panel->SetContent(Box);

	UTextBlock* Title = JinzzaUI::MakeTitleText(WidgetTree, TEXT("LobbyTitle"), FText::FromString(TEXT("LOBBY")), 30);
	UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(Title);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	SettingsText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("SettingsText"), FText::GetEmpty(), true);
	UVerticalBoxSlot* SettingsSlot = Box->AddChildToVerticalBox(SettingsText);
	SettingsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));

	PlayerCountText = JinzzaUI::MakeSectionHeading(WidgetTree, TEXT("PlayerCountText"), FText::GetEmpty());
	UVerticalBoxSlot* CountSlot = Box->AddChildToVerticalBox(PlayerCountText);
	CountSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));

	UButton* InviteButton = JinzzaUI::MakePrimaryButton(WidgetTree, TEXT("InviteButton"), FText::FromString(TEXT("Invite Friends")), 18.f);
	InviteButton->OnClicked.AddDynamic(this, &UjinzzaLobbyWidget::OnInviteFriendsClicked);
	UVerticalBoxSlot* InviteSlot = Box->AddChildToVerticalBox(InviteButton);
	InviteSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	StartButton = JinzzaUI::MakePrimaryButton(WidgetTree, TEXT("StartButton"), FText::FromString(TEXT("Start Match")), 18.f);
	StartButton->OnClicked.AddDynamic(this, &UjinzzaLobbyWidget::OnStartMatchClicked);
	Box->AddChildToVerticalBox(StartButton);

	const APlayerController* PC = GetOwningPlayer();
	StartButton->SetVisibility(PC && PC->HasAuthority() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	// Bottom-center interaction prompt, hidden until a kiosk is nearby.
	InteractPromptText = JinzzaUI::MakeSectionHeading(WidgetTree, TEXT("InteractPromptText"), FText::GetEmpty());
	InteractPromptText->SetJustification(ETextJustify::Center);
	InteractPromptText->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* PromptSlot = RootCanvas->AddChildToCanvas(InteractPromptText);
	PromptSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
	PromptSlot->SetAlignment(FVector2D(0.5f, 1.f));
	PromptSlot->SetPosition(FVector2D(0.f, -60.f));
	PromptSlot->SetAutoSize(true);
}

void UjinzzaLobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (PlayerCountText)
	{
		int32 Count = 0;
		if (AGameStateBase* GameState = UGameplayStatics::GetGameState(this))
		{
			Count = GameState->PlayerArray.Num();
		}
		PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("Players connected: %d"), Count)));
	}

	if (SettingsText)
	{
		if (const AjinzzaLobbyGameState* LobbyGameState = Cast<AjinzzaLobbyGameState>(UGameplayStatics::GetGameState(this)))
		{
			const FJinzzaMatchSettings& Settings = LobbyGameState->MatchSettings;
			SettingsText->SetText(FText::FromString(FString::Printf(
				TEXT("%s\nMax Players: %d | Judges: %d | Votes: %d\nPhase Speed: %s | Roles: %s"),
				*Settings.RoomName, Settings.MaxPlayers, Settings.JudgeCount, Settings.VoteCount,
				*Settings.PhaseSpeed, *Settings.RoleAssignMethod)));
		}
	}
}

void UjinzzaLobbyWidget::SetInteractionPrompt(const FText& PromptText)
{
	if (!InteractPromptText)
	{
		return;
	}

	if (PromptText.IsEmpty())
	{
		InteractPromptText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		InteractPromptText->SetText(PromptText);
		InteractPromptText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UjinzzaLobbyWidget::OnStartMatchClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(TEXT("/Game/JINZZA/Level/Lvl_Game"));
		}
	}
}

void UjinzzaLobbyWidget::OnInviteFriendsClicked()
{
	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->InviteFriends();
	}
}
