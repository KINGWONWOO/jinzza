// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaFriendInviteWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UScrollBox;
struct FJinzzaFriendInfo;
class UjinzzaFriendInviteWidget;

/** Binds one friend row's Invite button to that friend's NetId - a per-row closure object since
 * UButton::OnClicked is a zero-arg dynamic delegate (can't carry which friend was clicked
 * directly). Mirrors UJinzzaUIButtonSounds's role in jinzzaUIStyle.h. Not for direct use. */
UCLASS()
class UJinzzaFriendInviteRowHandler : public UObject
{
	GENERATED_BODY()

public:
	FString NetIdString;

	UPROPERTY()
	TWeakObjectPtr<UjinzzaFriendInviteWidget> OwnerWidget;

	UFUNCTION()
	void HandleInviteClicked();
};

/**
 * Centered "Invite Friends" panel opened by AjinzzaFriendInviteKiosk: fetches and shows the
 * local player's Steam friends list (UjinzzaGameInstance::RequestFriendsList), one row per
 * friend with an Invite button on the right (UjinzzaGameInstance::InviteFriendToSession), plus
 * Refresh and Close.
 *
 * C++-built (see docs/umg_widget_authoring_guide.md's pattern): the friend list is inherently
 * dynamic (unknown row count at compile time, rebuilt on every fetch), so it was never a
 * BindWidget candidate to begin with - only the static chrome (title/HeaderNote/Refresh/Close)
 * would move to a Designer-authored WBP if this project's UMG-authoring approach changes later.
 */
UCLASS()
class JINZZA_API UjinzzaFriendInviteWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	/** Called by UJinzzaFriendInviteRowHandler when that friend's Invite button is clicked. */
	void InviteFriend(const FString& NetIdString);

protected:
	UFUNCTION()
	void OnRefreshClicked();

	UFUNCTION()
	void OnCloseClicked();

private:
	void BuildWidgetTree();
	void RequestFriends();
	void HandleFriendsListReceived(const TArray<FJinzzaFriendInfo>& Friends);

	UPROPERTY()
	TObjectPtr<UTextBlock> HeaderNote;

	UPROPERTY()
	TObjectPtr<UScrollBox> FriendListScrollBox;

	UPROPERTY()
	TObjectPtr<UVerticalBox> FriendListBox;

	UPROPERTY()
	TObjectPtr<UButton> RefreshButton;

	UPROPERTY()
	TObjectPtr<UButton> CloseButton;

	/** Keeps every dynamically-constructed friend-row widget's name unique for this widget's
	 * whole lifetime, even across repeated Refresh clicks (ClearChildren() detaches old rows from
	 * display but doesn't immediately destroy the underlying objects, so reusing a name risks a
	 * NewObject collision with an orphaned-but-not-yet-GC'd one from a previous refresh). */
	int32 NextRowId = 0;

	FDelegateHandle FriendsListReceivedHandle;
};
