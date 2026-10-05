// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaRoomSettingsWidget.h"
#include "jinzzaUIStyle.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "jinzzaGameInstance.h"
#include "jinzzaLobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"

void UjinzzaRoomSettingsWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Sticker-style panel, matching the main menu / T_Logo.
	UBorder* PanelFace = nullptr;
	UOverlay* Panel = JinzzaUI::MakeStickerPanel(WidgetTree, TEXT("Panel"), PanelFace);
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* PanelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelBox"));
	PanelBox->SetWidthOverride(900.f);
	PanelFace->SetContent(PanelBox);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));
	PanelBox->AddChild(Stack);

	HeaderNote = JinzzaUI::MakeStickerHeading(WidgetTree, TEXT("HeaderNote"), FText::GetEmpty(), 44);
	JinzzaUI::AddSpaced(Stack, HeaderNote, 0.f);
	JinzzaUI::AddStickerSection(WidgetTree, Stack, TEXT("RoomSection"), FText::FromString(TEXT("Room")), true);

	RoomNameBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RoomNameBox"));
	JinzzaUI::ApplyStickerStyle(RoomNameBox);
	JinzzaUI::AddStickerRow(WidgetTree, Stack, TEXT("RoomNameRow"), FText::FromString(TEXT("Room Name")), RoomNameBox, 380.f);

	MaxPlayersSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("MaxPlayersSpinBox"));
	JinzzaUI::ApplyStickerStyle(MaxPlayersSpinBox);
	JinzzaUI::AddStickerRow(WidgetTree, Stack, TEXT("MaxPlayersRow"), FText::FromString(TEXT("Max Players")), MaxPlayersSpinBox, 380.f);

	JudgeCountSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("JudgeCountSpinBox"));
	JinzzaUI::ApplyStickerStyle(JudgeCountSpinBox);
	JinzzaUI::AddStickerRow(WidgetTree, Stack, TEXT("JudgeCountRow"), FText::FromString(TEXT("Judge Count")), JudgeCountSpinBox, 380.f);

	VoteCountSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("VoteCountSpinBox"));
	JinzzaUI::ApplyStickerStyle(VoteCountSpinBox);
	JinzzaUI::AddStickerRow(WidgetTree, Stack, TEXT("VoteCountRow"), FText::FromString(TEXT("Vote Count")), VoteCountSpinBox, 380.f);

	QuestionTimeCyclesSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("QuestionTimeCyclesSpinBox"));
	JinzzaUI::ApplyStickerStyle(QuestionTimeCyclesSpinBox);
	JinzzaUI::AddStickerRow(WidgetTree, Stack, TEXT("QuestionTimeCyclesRow"), FText::FromString(TEXT("Question Time Cycles")), QuestionTimeCyclesSpinBox, 380.f);

	PhaseSpeedCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PhaseSpeedCombo"));
	JinzzaUI::ApplyStickerStyle(PhaseSpeedCombo);
	JinzzaUI::AddStickerRow(WidgetTree, Stack, TEXT("PhaseSpeedRow"), FText::FromString(TEXT("Phase Speed")), PhaseSpeedCombo, 380.f);

	RoleAssignCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("RoleAssignCombo"));
	JinzzaUI::ApplyStickerStyle(RoleAssignCombo);
	JinzzaUI::AddStickerRow(WidgetTree, Stack, TEXT("RoleAssignRow"), FText::FromString(TEXT("Role Assign Method")), RoleAssignCombo, 380.f);

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	if (UVerticalBoxSlot* ButtonRowSlot = JinzzaUI::AddSpaced(Stack, ButtonRow, 28.f))
	{
		ButtonRowSlot->SetHorizontalAlignment(HAlign_Right);
	}

	CloseButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("CloseButton"), FText::FromString(TEXT("Close")), JinzzaUI::Sticker_Sky, 26.f);
	if (UHorizontalBoxSlot* CloseSlot = ButtonRow->AddChildToHorizontalBox(CloseButton))
	{
		CloseSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	ApplyButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("ApplyButton"), FText::FromString(TEXT("Apply")), JinzzaUI::Sticker_Yellow, 26.f, true);
	if (UHorizontalBoxSlot* ApplySlot = ButtonRow->AddChildToHorizontalBox(ApplyButton))
	{
		ApplySlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	StartGameButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("StartGameButton"), FText::FromString(TEXT("Start Game")), JinzzaUI::Sticker_Teal, 28.f, true);
	ButtonRow->AddChildToHorizontalBox(StartGameButton);
}

void UjinzzaRoomSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

	const APlayerController* OwningPC = GetOwningPlayer();
	bEditable = OwningPC && OwningPC->HasAuthority();

	if (HeaderNote)
	{
		HeaderNote->SetText(bEditable
			? FText::FromString(TEXT("You're the host - changes apply to everyone immediately."))
			: FText::FromString(TEXT("Only the host can change these settings.")));
	}

	const AjinzzaLobbyGameState* LobbyGameState = Cast<AjinzzaLobbyGameState>(UGameplayStatics::GetGameState(this));
	const FJinzzaMatchSettings CurrentSettings = LobbyGameState ? LobbyGameState->MatchSettings : FJinzzaMatchSettings();

	if (RoomNameBox)
	{
		RoomNameBox->SetText(FText::FromString(CurrentSettings.RoomName));
		RoomNameBox->SetIsReadOnly(!bEditable);
	}

	if (MaxPlayersSpinBox)
	{
		MaxPlayersSpinBox->SetMinValue(4.f);
		MaxPlayersSpinBox->SetMaxValue(12.f);
		MaxPlayersSpinBox->SetMinSliderValue(4.f);
		MaxPlayersSpinBox->SetMaxSliderValue(12.f);
		MaxPlayersSpinBox->SetValue(static_cast<float>(CurrentSettings.MaxPlayers));
		MaxPlayersSpinBox->SetDelta(1.f);
		MaxPlayersSpinBox->SetIsEnabled(bEditable);
	}

	if (JudgeCountSpinBox)
	{
		JudgeCountSpinBox->SetMinValue(1.f);
		JudgeCountSpinBox->SetMaxValue(2.f);
		JudgeCountSpinBox->SetMinSliderValue(1.f);
		JudgeCountSpinBox->SetMaxSliderValue(2.f);
		JudgeCountSpinBox->SetValue(static_cast<float>(CurrentSettings.JudgeCount));
		JudgeCountSpinBox->SetDelta(1.f);
		JudgeCountSpinBox->SetIsEnabled(bEditable);
	}

	if (VoteCountSpinBox)
	{
		VoteCountSpinBox->SetMinValue(1.f);
		VoteCountSpinBox->SetMaxValue(3.f);
		VoteCountSpinBox->SetMinSliderValue(1.f);
		VoteCountSpinBox->SetMaxSliderValue(3.f);
		VoteCountSpinBox->SetValue(static_cast<float>(CurrentSettings.VoteCount));
		VoteCountSpinBox->SetDelta(1.f);
		VoteCountSpinBox->SetIsEnabled(bEditable);
	}

	if (QuestionTimeCyclesSpinBox)
	{
		QuestionTimeCyclesSpinBox->SetMinValue(1.f);
		QuestionTimeCyclesSpinBox->SetMaxValue(3.f);
		QuestionTimeCyclesSpinBox->SetMinSliderValue(1.f);
		QuestionTimeCyclesSpinBox->SetMaxSliderValue(3.f);
		QuestionTimeCyclesSpinBox->SetValue(static_cast<float>(CurrentSettings.QuestionTimeCycles));
		QuestionTimeCyclesSpinBox->SetDelta(1.f);
		QuestionTimeCyclesSpinBox->SetIsEnabled(bEditable);
	}

	if (PhaseSpeedCombo)
	{
		PhaseSpeedCombo->AddOption(TEXT("Slow"));
		PhaseSpeedCombo->AddOption(TEXT("Normal"));
		PhaseSpeedCombo->AddOption(TEXT("Fast"));
		PhaseSpeedCombo->SetSelectedOption(CurrentSettings.PhaseSpeed.IsEmpty() ? TEXT("Normal") : CurrentSettings.PhaseSpeed);
		PhaseSpeedCombo->SetIsEnabled(bEditable);
	}

	if (RoleAssignCombo)
	{
		RoleAssignCombo->AddOption(TEXT("Random"));
		RoleAssignCombo->AddOption(TEXT("Host Picks"));
		RoleAssignCombo->SetSelectedOption(CurrentSettings.RoleAssignMethod.IsEmpty() ? TEXT("Random") : CurrentSettings.RoleAssignMethod);
		RoleAssignCombo->SetIsEnabled(bEditable);
	}

	if (ApplyButton)
	{
		ApplyButton->OnClicked.AddDynamic(this, &UjinzzaRoomSettingsWidget::OnApplyClicked);
		ApplyButton->SetVisibility(bEditable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UjinzzaRoomSettingsWidget::OnCloseClicked);
	}

	if (StartGameButton)
	{
		StartGameButton->OnClicked.AddDynamic(this, &UjinzzaRoomSettingsWidget::OnStartGameClicked);
		StartGameButton->SetVisibility(bEditable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UjinzzaRoomSettingsWidget::OnApplyClicked()
{
	if (!bEditable)
	{
		return;
	}

	AjinzzaLobbyGameState* LobbyGameState = Cast<AjinzzaLobbyGameState>(UGameplayStatics::GetGameState(this));
	if (!LobbyGameState)
	{
		return;
	}

	FJinzzaMatchSettings NewSettings;
	NewSettings.RoomName = RoomNameBox && !RoomNameBox->GetText().IsEmpty() ? RoomNameBox->GetText().ToString() : TEXT("JINZZA Room");
	NewSettings.MaxPlayers = MaxPlayersSpinBox ? FMath::RoundToInt(MaxPlayersSpinBox->GetValue()) : 6;
	NewSettings.JudgeCount = JudgeCountSpinBox ? FMath::RoundToInt(JudgeCountSpinBox->GetValue()) : 1;
	NewSettings.VoteCount = VoteCountSpinBox ? FMath::RoundToInt(VoteCountSpinBox->GetValue()) : 1;
	NewSettings.QuestionTimeCycles = QuestionTimeCyclesSpinBox ? FMath::RoundToInt(QuestionTimeCyclesSpinBox->GetValue()) : 2;
	NewSettings.PhaseSpeed = PhaseSpeedCombo ? PhaseSpeedCombo->GetSelectedOption() : TEXT("Normal");
	NewSettings.RoleAssignMethod = RoleAssignCombo ? RoleAssignCombo->GetSelectedOption() : TEXT("Random");

	LobbyGameState->MatchSettings = NewSettings;
	LobbyGameState->ForceNetUpdate();

	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		// Keep the pending settings snapshot (used if the host re-hosts later) in sync...
		GI->SetPendingMatchSettings(NewSettings);
		// ...and push the change (player count in particular) to the live Steam session so it's
		// actually what's advertised to friends, not just what's replicated to players already in.
		GI->UpdateLiveSessionSettings(NewSettings);
	}
}

void UjinzzaRoomSettingsWidget::OnCloseClicked()
{
	RemoveFromParent();
}

void UjinzzaRoomSettingsWidget::OnStartGameClicked()
{
	if (!bEditable)
	{
		return;
	}

	APlayerController* OwningPC = GetOwningPlayer();
	if (!OwningPC || !OwningPC->IsLocalController() || !OwningPC->HasAuthority())
	{
		return;
	}

	// TEMP placeholder confirm sound - matches AjinzzaStartMatchKiosk::Interact, swap for real SFX later.
	if (USoundBase* ConfirmSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/UISounds/LobbyPannelOpen__cut_1sec_.LobbyPannelOpen__cut_1sec_")))
	{
		UGameplayStatics::PlaySound2D(OwningPC, ConfirmSound);
	}

	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(TEXT("/Game/JINZZA/Level/Lvl_Game"));
	}
}
