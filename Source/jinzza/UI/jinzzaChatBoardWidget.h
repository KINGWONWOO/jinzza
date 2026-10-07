// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaChatBoardWidget.generated.h"

class UTextBlock;

/** The chalk text on a chat board (UjinzzaChatBoardComponent's world-space widget). */
UCLASS()
class JINZZA_API UjinzzaChatBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	void SetBoardText(const FString& Text);

private:
	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<UTextBlock> BoardText;
};
