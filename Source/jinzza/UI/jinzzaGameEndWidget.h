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
 * TEMP C++-built (see docs/umg_widget_authoring_guide.md): builds its own tree in
 * BuildWidgetTree() instead of relying on a Designer-authored WBP_GameEnd layout, so PIE isn't
 * blocked while the visual design pass hasn't happened yet. When that pass happens, delete
 * BuildWidgetTree(), restore `meta = (BindWidget)` on EndGameButton, and lay it out for real in
 * WBP_GameEnd's Designer per the guide.
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
	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<UButton> EndGameButton;
};
