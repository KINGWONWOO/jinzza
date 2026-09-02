// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaGameEndWidget.generated.h"

class UButton;

/**
 * Minimal in-round overlay: a host-only "End Game" button that returns everyone to
 * Lvl_Lobby. Stands in for a real win-condition trigger, which isn't implemented yet.
 */
UCLASS()
class JINZZA_API UjinzzaGameEndWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

protected:
	UFUNCTION()
	void OnEndGameClicked();

private:
	UPROPERTY()
	TObjectPtr<UButton> EndGameButton;
};
