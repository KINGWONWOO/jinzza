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
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md): builds its own tree in
 * BuildWidgetTree() instead of relying on a Designer-authored WBP_RoomSettings layout, so PIE
 * isn't blocked while the visual design pass hasn't happened yet. When that pass happens,
 * delete BuildWidgetTree(), restore `meta = (BindWidget)` on every property below, and lay them
 * out for real in WBP_RoomSettings's Designer per the guide.
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

	/** Host-only, mirrors AjinzzaStartMatchKiosk::Interact exactly - server-travels everyone to Lvl_Game. */
	UFUNCTION()
	void OnStartGameClicked();

private:
	void BuildWidgetTree();

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
	TObjectPtr<USpinBox> QuestionTimeCyclesSpinBox;

	UPROPERTY()
	TObjectPtr<UComboBoxString> PhaseSpeedCombo;

	UPROPERTY()
	TObjectPtr<UComboBoxString> RoleAssignCombo;

	UPROPERTY()
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY()
	TObjectPtr<UButton> CloseButton;

	UPROPERTY()
	TObjectPtr<UButton> StartGameButton;
};
