// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaVoteWidget.generated.h"

class UButton;
class UVerticalBox;
class UWidget;
class APlayerState;
class UjinzzaVoteWidget;

/** Click target for one candidate button (dynamic delegates need a UFUNCTION per bound object). */
UCLASS()
class UJinzzaVoteButtonHandler : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleClicked();

	TWeakObjectPtr<UjinzzaVoteWidget> OwnerWidget;
	TWeakObjectPtr<APlayerState> Candidate;
};

/**
 * The Judge's ballot (mid-evaluation and final decision): one button per living candidate, "User1".."UserN".
 * Only shown to the Judge while AjinzzaGameGameState's vote is open. Clicking sends
 * AjinzzaGamePlayerController::Server_CastVote; the vote closes as soon as every Judge has voted (the top
 * candidate then gets a final argument and is eliminated - see AjinzzaGameGameMode::CloseVote).
 *
 * Refreshed every frame by the controller.
 */
UCLASS()
class JINZZA_API UjinzzaVoteWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	void Refresh();

	/** The ballot is on screen (a vote is open and this player is a Judge). */
	bool IsShowing() const { return bWasOpen; }

	void Vote(APlayerState* Candidate);

private:
	void BuildWidgetTree();
	void RebuildButtons(const TArray<APlayerState*>& Candidates);

	UPROPERTY()
	TObjectPtr<UWidget> Panel;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ButtonList;

	UPROPERTY()
	TArray<TObjectPtr<UJinzzaVoteButtonHandler>> Handlers;

	TArray<TWeakObjectPtr<APlayerState>> ShownCandidates;
	TArray<TWeakObjectPtr<UButton>> CandidateButtons;
	TWeakObjectPtr<APlayerState> VotedFor;
	bool bWasOpen = false;
};
