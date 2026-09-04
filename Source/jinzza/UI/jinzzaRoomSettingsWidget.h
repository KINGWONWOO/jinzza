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
 *
 * UMG-authored: every property below must exist in this class's Widget Blueprint (e.g.
 * WBP_RoomSettings), named exactly as below, for BindWidget to find it. Row labels are
 * purely decorative and don't need to be bound - just placed next to each control in the
 * Designer.
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

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HeaderNote;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> RoomNameBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpinBox> MaxPlayersSpinBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpinBox> JudgeCountSpinBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USpinBox> VoteCountSpinBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> PhaseSpeedCombo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> RoleAssignCombo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;
};
