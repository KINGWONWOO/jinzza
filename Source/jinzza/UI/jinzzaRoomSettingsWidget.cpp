// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaRoomSettingsWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/ComboBoxString.h"
#include "jinzzaGameInstance.h"
#include "jinzzaLobbyGameState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void UjinzzaRoomSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

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
