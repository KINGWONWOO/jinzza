// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyWidget.h"
#include "jinzzaUIStyle.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "jinzzaLobbyGameState.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

namespace
{
	UVerticalBoxSlot* AddSpaced(UVerticalBox* Box, UWidget* Child, float TopPadding = 8.f)
	{
		UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child);
		if (Slot)
		{
			Slot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
		}
		return Slot;
	}
}

void UjinzzaLobbyWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* InfoPanel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("InfoPanel"));
	InfoPanel->SetPadding(FMargin(20.f));
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(InfoPanel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Left);
		PanelSlot->SetVerticalAlignment(VAlign_Top);
		PanelSlot->SetPadding(FMargin(24.f));
	}

	USizeBox* InfoBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InfoBox"));
	InfoBox->SetWidthOverride(360.f);
	InfoPanel->SetContent(InfoBox);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InfoStack"));
	InfoBox->AddChild(Stack);

	AddSpaced(Stack, JinzzaUI::MakeTitleText(WidgetTree, TEXT("LobbyTitle"), FText::FromString(TEXT("Lobby")), 28), 0.f);
	AddSpaced(Stack, JinzzaUI::MakeDivider(WidgetTree, TEXT("TitleDivider")));
	AddSpaced(Stack, JinzzaUI::MakeSectionHeading(WidgetTree, TEXT("RoomHeading"), FText::FromString(TEXT("Room"))), 16.f);

	SettingsText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("SettingsText"), FText::GetEmpty());
	AddSpaced(Stack, SettingsText, 6.f);

	PlayerCountText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("PlayerCountText"), FText::GetEmpty(), true);
	AddSpaced(Stack, PlayerCountText, 4.f);

	InteractPromptText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("InteractPromptText"), FText::GetEmpty());
	if (UOverlaySlot* PromptSlot = Root->AddChildToOverlay(InteractPromptText))
	{
		PromptSlot->SetHorizontalAlignment(HAlign_Center);
		PromptSlot->SetVerticalAlignment(VAlign_Bottom);
		PromptSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 60.f));
	}
}

void UjinzzaLobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

	if (InteractPromptText)
	{
		InteractPromptText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (USoundBase* Bgm = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/Lobby/LobbyBgm__cut_83sec__Cue.LobbyBgm__cut_83sec__Cue")))
	{
		MusicComponent = UGameplayStatics::SpawnSound2D(this, Bgm, 1.f, 1.f, 0.f, nullptr, true, false);
	}
}

void UjinzzaLobbyWidget::NativeDestruct()
{
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}

	Super::NativeDestruct();
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

