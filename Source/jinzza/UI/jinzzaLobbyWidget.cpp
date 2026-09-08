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
#include "Brushes/SlateColorBrush.h"

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

	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeTitleText(WidgetTree, TEXT("LobbyTitle"), FText::FromString(TEXT("Lobby")), 28), 0.f);
	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeDivider(WidgetTree, TEXT("TitleDivider")));
	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeSectionHeading(WidgetTree, TEXT("RoomHeading"), FText::FromString(TEXT("Room"))), 16.f);

	SettingsText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("SettingsText"), FText::GetEmpty());
	JinzzaUI::AddSpaced(Stack, SettingsText, 6.f);

	PlayerCountText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("PlayerCountText"), FText::GetEmpty(), true);
	JinzzaUI::AddSpaced(Stack, PlayerCountText, 4.f);

	InteractPromptText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("InteractPromptText"), FText::GetEmpty());
	if (UOverlaySlot* PromptSlot = Root->AddChildToOverlay(InteractPromptText))
	{
		PromptSlot->SetHorizontalAlignment(HAlign_Center);
		PromptSlot->SetVerticalAlignment(VAlign_Bottom);
		PromptSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 60.f));
	}

	// Welcome overlay shown once when the lobby first opens - see NativeOnInitialized/HideIntro.
	// Added last so it draws on top of InfoPanel/InteractPromptText.
	UBorder* IntroScrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("IntroScrim"));
	IntroScrim->SetBrush(FSlateColorBrush(FLinearColor(0.f, 0.f, 0.f, 0.75f)));
	IntroScrim->SetHorizontalAlignment(HAlign_Center);
	IntroScrim->SetVerticalAlignment(VAlign_Center);
	if (UOverlaySlot* ScrimSlot = Root->AddChildToOverlay(IntroScrim))
	{
		ScrimSlot->SetHorizontalAlignment(HAlign_Fill);
		ScrimSlot->SetVerticalAlignment(VAlign_Fill);
	}
	IntroOverlay = IntroScrim;

	UBorder* IntroPanel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("IntroPanel"));
	IntroPanel->SetPadding(FMargin(36.f));
	IntroScrim->SetContent(IntroPanel);

	USizeBox* IntroBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IntroBox"));
	IntroBox->SetWidthOverride(520.f);
	IntroPanel->SetContent(IntroBox);

	UVerticalBox* IntroStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("IntroStack"));
	IntroBox->AddChild(IntroStack);

	JinzzaUI::AddSpaced(IntroStack, JinzzaUI::MakeTitleText(WidgetTree, TEXT("IntroTitle"), FText::FromString(TEXT("JINZZA")), 36), 0.f);
	JinzzaUI::AddSpaced(IntroStack, JinzzaUI::MakeDivider(WidgetTree, TEXT("IntroDivider")));

	UTextBlock* IntroBody = JinzzaUI::MakeBodyText(WidgetTree, TEXT("IntroBody"), FText::FromString(TEXT(
		"Welcome to the lobby. Wait for everyone to join, visit the Wardrobe kiosk to customize "
		"your look, and press E at any kiosk to interact. The host can invite friends and start "
		"the match when ready.")));
	IntroBody->SetAutoWrapText(true);
	IntroBody->SetJustification(ETextJustify::Center);
	JinzzaUI::AddSpaced(IntroStack, IntroBody, 16.f);

	UTextBlock* IntroHint = JinzzaUI::MakeBodyText(WidgetTree, TEXT("IntroHint"), FText::FromString(TEXT("This will close automatically...")), true);
	IntroHint->SetJustification(ETextJustify::Center);
	JinzzaUI::AddSpaced(IntroStack, IntroHint, 16.f);
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

	if (IntroOverlay)
	{
		GetWorld()->GetTimerManager().SetTimer(IntroTimerHandle, this, &UjinzzaLobbyWidget::HideIntro, 5.f, false);
	}

	// TEMP placeholder one-shot entry sound - swap for real SFX later.
	if (USoundBase* EntrySound = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/UISounds/LobbyPannelOpen__cut_1sec_.LobbyPannelOpen__cut_1sec_")))
	{
		UGameplayStatics::PlaySound2D(this, EntrySound);
	}
}

void UjinzzaLobbyWidget::NativeDestruct()
{
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(IntroTimerHandle);
	}

	Super::NativeDestruct();
}

void UjinzzaLobbyWidget::HideIntro()
{
	if (IntroOverlay)
	{
		IntroOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
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
				TEXT("%s\nMax Players: %d | Judges: %d | Votes: %d | Question Cycles: %d\nPhase Speed: %s | Roles: %s"),
				*Settings.RoomName, Settings.MaxPlayers, Settings.JudgeCount, Settings.VoteCount,
				Settings.QuestionTimeCycles, *Settings.PhaseSpeed, *Settings.RoleAssignMethod)));
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

