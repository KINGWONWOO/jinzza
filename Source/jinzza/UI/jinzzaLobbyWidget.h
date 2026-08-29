// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaLobbyWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * Pre-match lobby UI: shows connected player count and a host-only "Start Match"
 * button that server-travels everyone to Lvl_Game.
 */
UCLASS()
class JINZZA_API UjinzzaLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UFUNCTION()
	void OnStartMatchClicked();

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> PlayerCountText;

	UPROPERTY()
	TObjectPtr<UButton> StartButton;
};
