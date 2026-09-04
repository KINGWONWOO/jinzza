// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaGameEndWidget.generated.h"

class UButton;

/**
 * Minimal in-round overlay: a host-only "End Game" button that returns everyone to
 * Lvl_Lobby. Stands in for a real win-condition trigger, which isn't implemented yet.
 *
 * UMG-authored: this class only wires up logic onto widgets built in a Widget Blueprint
 * (e.g. WBP_GameEnd) that subclasses this. EndGameButton must exist in that Blueprint's
 * widget tree, named exactly as below, for BindWidget to find it.
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
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> EndGameButton;
};
