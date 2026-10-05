// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyWidget.h"
#include "jinzzaUIStyle.h"
#include "jinzzaInputKeys.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"

void UjinzzaLobbyWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Top-left room info card - a black sticker (white outline + drop shadow), matching the main
	// menu / T_Logo.
	UBorder* InfoFace = nullptr;
	UOverlay* InfoPanel = JinzzaUI::MakeSticker(WidgetTree, TEXT("InfoPanel"), JinzzaUI::Sticker_Ink, 24.f, InfoFace);
	InfoFace->SetPadding(FMargin(22.f, 16.f, 22.f, 18.f));
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(InfoPanel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Left);
		PanelSlot->SetVerticalAlignment(VAlign_Top);
		PanelSlot->SetPadding(FMargin(24.f));
	}

	USizeBox* InfoBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InfoBox"));
	InfoBox->SetWidthOverride(420.f);
	InfoFace->SetContent(InfoBox);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InfoStack"));
	InfoBox->AddChild(Stack);

	// "Lobby" title with a yellow dot, sticker-label style.
	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LobbyTitleRow"));
	if (UHorizontalBoxSlot* DotSlot = TitleRow->AddChildToHorizontalBox(JinzzaUI::MakeStickerDot(WidgetTree, TEXT("LobbyTitleDot"), JinzzaUI::Sticker_Yellow, 16.f)))
	{
		DotSlot->SetVerticalAlignment(VAlign_Center);
		DotSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}
	if (UHorizontalBoxSlot* TitleSlot = TitleRow->AddChildToHorizontalBox(JinzzaUI::MakeStickerHeading(WidgetTree, TEXT("LobbyTitle"), FText::FromString(TEXT("Lobby")), 38)))
	{
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}
	JinzzaUI::AddSpaced(Stack, TitleRow, 0.f);

	// Auto-wrap so a long room name or settings line stays inside the 420px card instead of
	// running past its right edge.
	SettingsText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("SettingsText"), FText::GetEmpty(), 20);
	SettingsText->SetAutoWrapText(true);
	JinzzaUI::AddSpaced(Stack, SettingsText, 10.f);

	PlayerCountText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("PlayerCountText"), FText::GetEmpty(), 20, true);
	PlayerCountText->SetAutoWrapText(true);
	JinzzaUI::AddSpaced(Stack, PlayerCountText, 4.f);

	// Same key cap + label look as the in-world prop prompt (UjinzzaInteractionPromptWidget).
	UBorder* PromptFace = nullptr;
	UOverlay* PromptPanel = JinzzaUI::MakeSticker(WidgetTree, TEXT("InteractPrompt"), JinzzaUI::Sticker_Ink, 26.f, PromptFace);
	PromptFace->SetPadding(FMargin(12.f, 8.f, 18.f, 8.f));
	if (UOverlaySlot* PromptSlot = Root->AddChildToOverlay(PromptPanel))
	{
		PromptSlot->SetHorizontalAlignment(HAlign_Center);
		PromptSlot->SetVerticalAlignment(VAlign_Bottom);
		PromptSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 60.f));
	}
	InteractPrompt = PromptPanel;

	UHorizontalBox* PromptRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InteractPromptRow"));
	PromptFace->SetContent(PromptRow);

	UTextBlock* KeyLabel = nullptr;
	UWidget* KeyCap = JinzzaUI::MakeStickerKeyCap(WidgetTree, TEXT("InteractKeyCap"), KeyLabel, 42.f);
	InteractKeyText = KeyLabel;
	if (UHorizontalBoxSlot* KeySlot = PromptRow->AddChildToHorizontalBox(KeyCap))
	{
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeySlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}

	InteractPromptText = JinzzaUI::MakeStickerText(WidgetTree, TEXT("InteractPromptText"), FText::GetEmpty(), 24);
	if (UHorizontalBoxSlot* TextSlot = PromptRow->AddChildToHorizontalBox(InteractPromptText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UjinzzaLobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

	if (InteractPrompt)
	{
		InteractPrompt->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (USoundBase* Bgm = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/Lobby/LobbyBgm__cut_83sec__Cue.LobbyBgm__cut_83sec__Cue")))
	{
		MusicComponent = UGameplayStatics::SpawnSound2D(this, Bgm, 1.f, 1.f, 0.f, nullptr, true, false);
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
				TEXT("%s\nMax Players: %d | Judges: %d | Votes: %d\nQuestion Cycles: %d\nPhase Speed: %s | Roles: %s"),
				*Settings.RoomName, Settings.MaxPlayers, Settings.JudgeCount, Settings.VoteCount,
				Settings.QuestionTimeCycles, *Settings.PhaseSpeed, *Settings.RoleAssignMethod)));
		}
	}
}

void UjinzzaLobbyWidget::SetInteractionPrompt(const FText& PromptText)
{
	if (!InteractPrompt || !InteractPromptText)
	{
		return;
	}

	if (PromptText.IsEmpty())
	{
		InteractPrompt->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		// Re-read every time it appears - the key may have been rebound in Settings since.
		if (InteractKeyText)
		{
			InteractKeyText->SetText(JinzzaInput::GetKeyCapText(JinzzaInput::GetBoundKey(JinzzaInput::ResolveLocalPlayer(this), JinzzaInput::GetInteractAction())));
		}
		InteractPromptText->SetText(PromptText);
		InteractPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

