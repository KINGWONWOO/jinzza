// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaRoomSettingsWidget.h"
#include "jinzzaUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/ComboBoxString.h"
#include "jinzzaGameInstance.h"
#include "jinzzaLobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

namespace
{
	void AddSettingLabel(UWidgetTree* Tree, UVerticalBox* Parent, const FString& RowName, const FText& Label)
	{
		Parent->AddChildToVerticalBox(JinzzaUI::MakeBodyText(Tree, *(RowName + TEXT("_Label")), Label, true));
	}
}

void UjinzzaRoomSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	const APlayerController* OwningPC = GetOwningPlayer();
	bEditable = OwningPC && OwningPC->HasAuthority();

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RoomSettingsRootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RoomSettingsDimmer"));
	Dimmer->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Dimmer->SetHorizontalAlignment(HAlign_Center);
	Dimmer->SetVerticalAlignment(VAlign_Center);
	UCanvasPanelSlot* DimmerSlot = RootCanvas->AddChildToCanvas(Dimmer);
	DimmerSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimmerSlot->SetOffsets(FMargin(0.f));

	UBorder* Panel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("RoomSettingsPanel"));
	Panel->SetPadding(FMargin(36.f, 28.f));
	Dimmer->SetContent(Panel);

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RoomSettingsBox"));
	Panel->SetContent(Box);

	UTextBlock* Title = JinzzaUI::MakeTitleText(WidgetTree, TEXT("RoomSettingsTitle"), FText::FromString(TEXT("ROOM SETTINGS")), 30);
	UVerticalBoxSlot* TitleSlot = Box->AddChildToVerticalBox(Title);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	HeaderNote = JinzzaUI::MakeBodyText(WidgetTree, TEXT("RoomSettingsNote"),
		bEditable
			? FText::FromString(TEXT("You're the host - changes apply to everyone immediately."))
			: FText::FromString(TEXT("Only the host can change these settings.")),
		true);
	HeaderNote->SetAutoWrapText(true);
	UVerticalBoxSlot* NoteSlot = Box->AddChildToVerticalBox(HeaderNote);
	NoteSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));

	const AjinzzaLobbyGameState* LobbyGameState = Cast<AjinzzaLobbyGameState>(UGameplayStatics::GetGameState(this));
	const FJinzzaMatchSettings CurrentSettings = LobbyGameState ? LobbyGameState->MatchSettings : FJinzzaMatchSettings();

	// Room name
	AddSettingLabel(WidgetTree, Box, TEXT("RoomName"), FText::FromString(TEXT("Room Name")));
	RoomNameBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("RoomNameBox"));
	RoomNameBox->SetText(FText::FromString(CurrentSettings.RoomName));
	RoomNameBox->SetIsReadOnly(!bEditable);
	Box->AddChildToVerticalBox(RoomNameBox);

	// Max players
	AddSettingLabel(WidgetTree, Box, TEXT("MaxPlayers"), FText::FromString(TEXT("Max Players (4-12)")));
	MaxPlayersSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("MaxPlayersSpinBox"));
	MaxPlayersSpinBox->SetMinValue(4.f);
	MaxPlayersSpinBox->SetMaxValue(12.f);
	MaxPlayersSpinBox->SetMinSliderValue(4.f);
	MaxPlayersSpinBox->SetMaxSliderValue(12.f);
	MaxPlayersSpinBox->SetValue(static_cast<float>(CurrentSettings.MaxPlayers));
	MaxPlayersSpinBox->SetDelta(1.f);
	MaxPlayersSpinBox->SetIsEnabled(bEditable);
	UVerticalBoxSlot* MaxPlayersSlot = Box->AddChildToVerticalBox(MaxPlayersSpinBox);
	MaxPlayersSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	// Judge count
	AddSettingLabel(WidgetTree, Box, TEXT("JudgeCount"), FText::FromString(TEXT("Judge Count (1-2)")));
	JudgeCountSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("JudgeCountSpinBox"));
	JudgeCountSpinBox->SetMinValue(1.f);
	JudgeCountSpinBox->SetMaxValue(2.f);
	JudgeCountSpinBox->SetMinSliderValue(1.f);
	JudgeCountSpinBox->SetMaxSliderValue(2.f);
	JudgeCountSpinBox->SetValue(static_cast<float>(CurrentSettings.JudgeCount));
	JudgeCountSpinBox->SetDelta(1.f);
	JudgeCountSpinBox->SetIsEnabled(bEditable);
	UVerticalBoxSlot* JudgeSlot = Box->AddChildToVerticalBox(JudgeCountSpinBox);
	JudgeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	// Vote count
	AddSettingLabel(WidgetTree, Box, TEXT("VoteCount"), FText::FromString(TEXT("Vote Count (1-3)")));
	VoteCountSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("VoteCountSpinBox"));
	VoteCountSpinBox->SetMinValue(1.f);
	VoteCountSpinBox->SetMaxValue(3.f);
	VoteCountSpinBox->SetMinSliderValue(1.f);
	VoteCountSpinBox->SetMaxSliderValue(3.f);
	VoteCountSpinBox->SetValue(static_cast<float>(CurrentSettings.VoteCount));
	VoteCountSpinBox->SetDelta(1.f);
	VoteCountSpinBox->SetIsEnabled(bEditable);
	UVerticalBoxSlot* VoteSlot = Box->AddChildToVerticalBox(VoteCountSpinBox);
	VoteSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	// Phase speed
	AddSettingLabel(WidgetTree, Box, TEXT("PhaseSpeed"), FText::FromString(TEXT("Phase Speed")));
	PhaseSpeedCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("PhaseSpeedCombo"));
	PhaseSpeedCombo->AddOption(TEXT("Slow"));
	PhaseSpeedCombo->AddOption(TEXT("Normal"));
	PhaseSpeedCombo->AddOption(TEXT("Fast"));
	PhaseSpeedCombo->SetSelectedOption(CurrentSettings.PhaseSpeed.IsEmpty() ? TEXT("Normal") : CurrentSettings.PhaseSpeed);
	PhaseSpeedCombo->SetIsEnabled(bEditable);
	UVerticalBoxSlot* PhaseSlot = Box->AddChildToVerticalBox(PhaseSpeedCombo);
	PhaseSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	// Role assign method
	AddSettingLabel(WidgetTree, Box, TEXT("RoleAssign"), FText::FromString(TEXT("Role Assign Method")));
	RoleAssignCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("RoleAssignCombo"));
	RoleAssignCombo->AddOption(TEXT("Random"));
	RoleAssignCombo->AddOption(TEXT("Host Picks"));
	RoleAssignCombo->SetSelectedOption(CurrentSettings.RoleAssignMethod.IsEmpty() ? TEXT("Random") : CurrentSettings.RoleAssignMethod);
	RoleAssignCombo->SetIsEnabled(bEditable);
	UVerticalBoxSlot* RoleSlot = Box->AddChildToVerticalBox(RoleAssignCombo);
	RoleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 22.f));

	// Buttons
	if (bEditable)
	{
		ApplyButton = JinzzaUI::MakePrimaryButton(WidgetTree, TEXT("RoomSettingsApplyButton"), FText::FromString(TEXT("Apply")));
		ApplyButton->OnClicked.AddDynamic(this, &UjinzzaRoomSettingsWidget::OnApplyClicked);
		UVerticalBoxSlot* ApplySlot = Box->AddChildToVerticalBox(ApplyButton);
		ApplySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	UButton* CloseButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("RoomSettingsCloseButton"), FText::FromString(TEXT("Close")));
	CloseButton->OnClicked.AddDynamic(this, &UjinzzaRoomSettingsWidget::OnCloseClicked);
	Box->AddChildToVerticalBox(CloseButton);
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
