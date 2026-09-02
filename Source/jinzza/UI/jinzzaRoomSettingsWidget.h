// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaRoomSettingsWidget.generated.h"

class UEditableTextBox;
class USpinBox;
class UComboBoxString;
class UTextBlock;
class UButton;

/**
 * Room settings panel opened by interacting with AjinzzaRoomSettingsKiosk in the lobby:
 * room name, max players, judge count, vote count, phase speed, and role-assign method.
 * Host-only: the fields are editable and an Apply button writes straight into
 * AjinzzaLobbyGameState::MatchSettings (this widget only ever runs on the host's own
 * client instance when editable, so no RPC is needed - matches the HasAuthority() +
 * IsLocalController() host-check idiom already used by UjinzzaLobbyWidget's Start Match
 * button). Non-host players see the same values read-only.
 */
UCLASS()
class JINZZA_API UjinzzaRoomSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

protected:
	UFUNCTION()
	void OnApplyClicked();

	UFUNCTION()
	void OnCloseClicked();

private:
	bool bEditable = false;

	UPROPERTY()
	TObjectPtr<UTextBlock> HeaderNote;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> RoomNameBox;

	UPROPERTY()
	TObjectPtr<USpinBox> MaxPlayersSpinBox;

	UPROPERTY()
	TObjectPtr<USpinBox> JudgeCountSpinBox;

	UPROPERTY()
	TObjectPtr<USpinBox> VoteCountSpinBox;

	UPROPERTY()
	TObjectPtr<UComboBoxString> PhaseSpeedCombo;

	UPROPERTY()
	TObjectPtr<UComboBoxString> RoleAssignCombo;

	UPROPERTY()
	TObjectPtr<UButton> ApplyButton;
};
