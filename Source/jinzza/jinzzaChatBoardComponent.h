// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "jinzzaChatBoardComponent.generated.h"

class USceneComponent;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EJinzzaChatBoardState : uint8
{
	/** Board lowered and hidden. */
	Hidden,
	/** Chat input is open: board raised, written side toward its owner - everyone else sees the blank back. */
	Writing,
	/** Message sent: board flipped round, written side toward everyone else, for a few seconds. */
	Showing,
};

USTRUCT()
struct FJinzzaChatBoardReplicated
{
	GENERATED_BODY()

	UPROPERTY()
	EJinzzaChatBoardState State = EJinzzaChatBoardState::Hidden;

	/** Only set while Showing - nobody gets the text before it's revealed. */
	UPROPERTY()
	FString Text;

	/** Question Time answer board: shows the owner's drawing (AjinzzaPartyPlayerState::GetLocalRevealedDrawing)
	 * instead of text, and stays up until lowered. */
	UPROPERTY()
	bool bAnswer = false;
};

/**
 * Chat as a hand-held chalkboard - replaces the old scrolling chat log (no record is kept anywhere).
 *
 * Enter opens the chat input: the character raises the board with its written side toward itself
 * (its owner sees the text live as they type, nobody else sees anything but the blank back). Enter
 * again sends: the board flips 180 degrees to face outward and everyone nearby can read it for
 * DisplaySeconds (longer for longer text), then it's lowered. ESC lowers it unwritten.
 *
 * MOCKUP visuals: an engine cube as the board and a lerped raise/flip instead of animation. When the
 * real writing animation exists, drive it from GetBoardState() (Writing = writing pose).
 *
 * State is server-authoritative (set through AjinzzaPlayerController's chat RPCs) and replicated; the
 * raise/flip motion runs locally on every machine. A ghost's board is only visible to other ghosts
 * (same rule as voice).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JINZZA_API UjinzzaChatBoardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UjinzzaChatBoardComponent();

	/** Called from the owning character's constructor: Pivot is moved/rotated, TextWidget shows the text. */
	void Bind(USceneComponent* InPivot, UWidgetComponent* InTextWidget);

	UFUNCTION(BlueprintPure, Category = "Chat")
	EJinzzaChatBoardState GetBoardState() const { return Board.State; }

	// Server-only.
	void ServerStartWriting();
	void ServerCancelWriting();
	/** Lowers the board whatever it's doing (a speaking turn starting). */
	void ServerHideNow();
	/** Text must already be cleaned/length-limited (AjinzzaPlayerController::Server_SendChatMessage). */
	void ServerReveal(const FString& Text);

	/** Question Time: raise the board as a sketchbook, blank side out, while its owner draws an answer. */
	void ServerHoldUpAnswer();
	/** Question Time: flip it round to show the answer drawing - stays up until ServerHideNow. */
	void ServerRevealAnswer();

	/** Local owner only: what they're typing, drawn on their own side of the board while Writing. */
	void SetLocalPreviewText(const FString& Text);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_Board();

	void SetBoard(EJinzzaChatBoardState NewState, const FString& NewText, bool bInAnswer = false);
	void ServerHide();
	void RefreshText();
	bool CanLocalViewerSee() const;

	UPROPERTY(ReplicatedUsing = OnRep_Board)
	FJinzzaChatBoardReplicated Board;

	UPROPERTY()
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY()
	TObjectPtr<UWidgetComponent> TextWidget;

	FVector PivotRestLocation = FVector::ZeroVector;
	FString LocalPreviewText;

	/** 0 = lowered, 1 = held up. */
	float Raise = 0.f;
	/** Board yaw relative to the character: 180 = written side toward the owner, 0 = toward everyone else. */
	float Yaw = 180.f;

	FTimerHandle HideTimerHandle;

	/** Last AjinzzaPartyPlayerState drawing revision pushed into the board widget (-1 = none). */
	int32 ShownDrawingRevision = -1;
};
