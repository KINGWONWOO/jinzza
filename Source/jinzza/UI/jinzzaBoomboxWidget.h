// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaBoomboxWidget.generated.h"

class AjinzzaBoomboxProp;
class UButton;
class UTextBlock;
class UVerticalBox;
class UEditableTextBox;
class UjinzzaBoomboxWidget;

/** Binds one song row's button to that song's playlist index (UButton::OnClicked carries no arguments). Not for direct use. */
UCLASS()
class UJinzzaBoomboxRowHandler : public UObject
{
	GENERATED_BODY()

public:
	int32 TrackIndex = -1;

	UPROPERTY()
	TWeakObjectPtr<UjinzzaBoomboxWidget> OwnerWidget;

	UFUNCTION()
	void HandleClicked();
};

/**
 * The boombox's music player panel: opened by AjinzzaCharacter when the holder presses the interact key
 * again while carrying a boombox (see AjinzzaBoomboxProp::CreateHeldInteractionWidget). Shows what's playing,
 * the built-in song list (left-click a song = play it; left-click the song that's already current = pause /
 * resume), a text box to paste a direct audio link, and Play/Pause + Close buttons.
 *
 * Purely a view + command sender: the state lives on the boombox (replicated), and commands go out through
 * AjinzzaCharacter::RequestBoomboxMusic. The panel refreshes itself whenever the boombox's state changes.
 * C++-built (same approach as UjinzzaFriendInviteWidget) since the song list is dynamic.
 */
UCLASS()
class JINZZA_API UjinzzaBoomboxWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void SetBoombox(AjinzzaBoomboxProp* InBoombox);

	/** Called by UJinzzaBoomboxRowHandler: left-click on a song row. */
	void OnTrackClicked(int32 TrackIndex);

protected:
	/** Swallows clicks on the dimmed backdrop so they don't fall through to the game (where left click means "use held prop"). */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	void OnPlayLinkClicked();

	UFUNCTION()
	void OnLinkCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnPlayPauseClicked();

	UFUNCTION()
	void OnCloseClicked();

private:
	void BuildWidgetTree();
	void HandleMusicStateChanged();
	void RefreshFromBoombox();
	void SubmitLink();
	void SetLinkMessage(const FString& Message, bool bError);

	UPROPERTY()
	TWeakObjectPtr<AjinzzaBoomboxProp> Boombox;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TObjectPtr<UVerticalBox> TrackListBox;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> LinkTextBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> LinkMessageText;

	UPROPERTY()
	TObjectPtr<UButton> PlayLinkButton;

	UPROPERTY()
	TObjectPtr<UButton> PlayPauseButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> PlayPauseLabel;

	UPROPERTY()
	TObjectPtr<UButton> CloseButton;

	/** Unique row names for the widget's whole lifetime (see UjinzzaFriendInviteWidget::NextRowId). */
	int32 NextRowId = 0;

	/** Set by the boombox's state-changed delegate, consumed in NativeTick: rebuilding the list from inside
	 * the very button click that caused the change would destroy that button mid-click. */
	bool bRefreshPending = true;

	FDelegateHandle MusicStateChangedHandle;
};
