// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaChatBoardWidget.generated.h"

class UTextBlock;
class UjinzzaSketchPad;
class USizeBox;
struct FJinzzaDrawing;

/** The chalk text on a chat board (UjinzzaChatBoardComponent's world-space widget). */
UCLASS()
class JINZZA_API UjinzzaChatBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	void SetBoardText(const FString& Text);

	/** Question Time answer board: bShow turns the board into a sketchbook page showing Drawing (null =
	 * blank paper; an empty drawing shows a big "?" - no answer). */
	void SetSketch(bool bShow, const FJinzzaDrawing* Drawing);

private:
	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<UTextBlock> BoardText;

	UPROPERTY()
	TObjectPtr<USizeBox> SketchBox;

	UPROPERTY()
	TObjectPtr<UjinzzaSketchPad> Sketch;

	UPROPERTY()
	TObjectPtr<UTextBlock> NoAnswerText;
};
