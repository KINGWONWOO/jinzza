// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaLobbyWidget.generated.h"

class UTextBlock;
class UButton;
class UAudioComponent;

/**
 * Pre-match lobby UI: shows the replicated match settings, connected player count,
 * an Invite Friends button, and a host-only "Start Match" button that server-travels
 * everyone to Lvl_Game.
 */
UCLASS()
class JINZZA_API UjinzzaLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Shows a bottom-center interaction prompt (e.g. "Press E - Room Settings"), or hides it if PromptText is empty. */
	void SetInteractionPrompt(const FText& PromptText);

protected:
	UFUNCTION()
	void OnStartMatchClicked();

	UFUNCTION()
	void OnInviteFriendsClicked();

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> SettingsText;

	UPROPERTY()
	TObjectPtr<UTextBlock> PlayerCountText;

	UPROPERTY()
	TObjectPtr<UButton> StartButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> InteractPromptText;

	/** Looping lobby BGM, started in NativeOnInitialized and stopped in NativeDestruct. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> MusicComponent;
};
