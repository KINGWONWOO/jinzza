// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaQuestionTypes.h"
#include "jinzzaQuestionWidget.generated.h"

class AActor;
class APlayerState;
class UBorder;
class UButton;
class UEditableTextBox;
class UHorizontalBox;
class UImage;
class UOverlay;
class UProgressBar;
class USceneCaptureComponent2D;
class UTextBlock;
class UTextureRenderTarget2D;
class UUniformGridPanel;
class UWidget;
class UjinzzaQuestionWidget;
class UjinzzaSketchPad;

/** What a Question Time button does (UJinzzaQuestionToolHandler). */
UENUM()
enum class EJinzzaQuestionTool : uint8
{
	Color,
	Size,
	Undo,
	Clear,
	SubmitDrawing,
	SubmitQuestion
};

/** Click target for one Question Time button (dynamic delegates need a UFUNCTION per bound object). */
UCLASS()
class UJinzzaQuestionToolHandler : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleClicked();

	TWeakObjectPtr<UjinzzaQuestionWidget> OwnerWidget;
	EJinzzaQuestionTool Tool = EJinzzaQuestionTool::Color;
	int32 Index = 0;
};

/** One seat in the split screen: the camera filming them and the panel showing it. */
USTRUCT()
struct FJinzzaQuestionPanel
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<APlayerState> Player;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> Target;

	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> Capture;

	UPROPERTY()
	TObjectPtr<UWidget> StatusChip;

	UPROPERTY()
	TObjectPtr<UBorder> StatusFace;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	/** Their answer board was showing last frame (to play the flip sound once). */
	bool bBoardShowing = false;
};

/**
 * Question Time HUD (created by AjinzzaGamePlayerController, refreshed every frame from the replicated
 * FJinzzaQuestionState):
 *  - Split screen: one panel per seat, each fed live by a local scene capture placed in front of that
 *    player (the camera the server computed for the seat). Shown whenever people interact - while the
 *    Judge asks, while the question drops, and while the answers are revealed and discussed.
 *  - Question sign: drops in from the top like a variety-show sign, swings and settles, with the step's
 *    countdown under it.
 *  - Ask box (Judge, Asking): type the question; Enter or "Ask!" sends it.
 *  - Answer screen (candidates, Answering): just you and a sketchbook - pick a marker color and size,
 *    draw, undo/clear, hand it in. Live feedback: the countdown (red + ticking in the last 5 s), who has
 *    answered so far, how much ink is left, and a stamp once it's in. Hands in whatever is drawn just
 *    before time runs out.
 */
UCLASS()
class JINZZA_API UjinzzaQuestionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	void Refresh(float DeltaTime);

	void HandleTool(EJinzzaQuestionTool Tool, int32 Index);

	/** True while this player has something to type or draw here (the ask box, the drawing screen) - the
	 * controller shows the cursor then (AjinzzaGamePlayerController::UpdateUICursor). */
	bool WantsCursor() const { return bWantsCursor; }

private:
	void BuildWidgetTree();
	UWidget* BuildSign();
	UWidget* BuildAskPanel();
	UWidget* BuildAnswerScreen();
	UButton* MakeTool(UHorizontalBox* Row, const FText& Label, const FLinearColor& Accent, EJinzzaQuestionTool Tool, int32 Index, float FontSize = 18.f);
	UButton* MakeSwatch(UHorizontalBox* Row, int32 ColorIndex);

	/** (Re)creates panels + scene captures when the seats change; tears everything down when Seats is empty. */
	void SyncPanels(const FJinzzaQuestionState& State);
	void DestroyPanels();
	void SetCapturesActive(bool bActive);

	void OnStepEntered(const FJinzzaQuestionState& State, bool bAsker, bool bAnswerer);
	void UpdateSign(const FJinzzaQuestionState& State, double ServerNow);
	void UpdatePanels(const FJinzzaQuestionState& State);
	void UpdateAnswerScreen(const FJinzzaQuestionState& State, float Remaining);
	void RefreshToolHighlights();

	void SubmitDrawing(bool bTimeUp);
	void SubmitQuestion();

	UFUNCTION()
	void HandleQuestionCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleQuestionChanged(const FText& Text);

	void PlaySound(const TCHAR* Path) const;

	/** Unique widget name: JinzzaUI's sticker helpers derive child names from it ("<Name>_Label"), so two
	 * unnamed (NAME_None) stickers would collide on "None_Label". */
	FName NextName(const TCHAR* Base);
	int32 NameCounter = 0;

	// --- Widgets ---

	UPROPERTY()
	TObjectPtr<UWidget> SplitLayer;

	UPROPERTY()
	TObjectPtr<UUniformGridPanel> Grid;

	UPROPERTY()
	TObjectPtr<UWidget> Banner;

	UPROPERTY()
	TObjectPtr<UTextBlock> BannerText;

	UPROPERTY()
	TObjectPtr<UWidget> SignRoot;

	UPROPERTY()
	TObjectPtr<UWidget> Sign;

	UPROPERTY()
	TObjectPtr<UTextBlock> SignText;

	UPROPERTY()
	TObjectPtr<UWidget> Countdown;

	UPROPERTY()
	TObjectPtr<UBorder> CountdownFace;

	UPROPERTY()
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY()
	TObjectPtr<UWidget> AskLayer;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> QuestionBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> AskCounter;

	UPROPERTY()
	TObjectPtr<UButton> AskButton;

	UPROPERTY()
	TObjectPtr<UWidget> AnswerLayer;

	UPROPERTY()
	TObjectPtr<UjinzzaSketchPad> Pad;

	UPROPERTY()
	TObjectPtr<UWidget> Stamp;

	UPROPERTY()
	TObjectPtr<UTextBlock> StampText;

	UPROPERTY()
	TObjectPtr<UTextBlock> AnswerClock;

	UPROPERTY()
	TObjectPtr<UTextBlock> AnsweredText;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> AnsweredDots;

	UPROPERTY()
	TObjectPtr<UProgressBar> InkBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> AnswerHint;

	UPROPERTY()
	TObjectPtr<UButton> SubmitButton;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> SwatchButtons;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> SizeButtons;

	UPROPERTY()
	TArray<TObjectPtr<UJinzzaQuestionToolHandler>> Handlers;

	// --- Split screen ---

	UPROPERTY()
	TArray<FJinzzaQuestionPanel> Panels;

	/** Local-only actor holding the panels' scene captures. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> CaptureRig;

	bool bCapturesActive = false;

	// --- Local state ---

	int32 ShownSerial = -1;
	EJinzzaQuestionStep ShownStep = EJinzzaQuestionStep::None;
	bool bSubmitted = false;
	bool bQuestionSent = false;
	bool bWantsCursor = false;
	int32 LastTickSecond = -1;
	int32 ShownAnsweredCount = -1;
	/** Seconds since the answer stamp appeared (pop-in animation), < 0 = hidden. */
	float StampAge = -1.f;
};
