// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaGameInstance.h"
#include "jinzzaMainMenuWidget.generated.h"

class UTextBlock;

/**
 * Main menu UI: title, Host/Join/Settings/Quit buttons, and a status line driven by
 * UjinzzaGameInstance's session status delegate. Built entirely in C++ (no separate
 * Widget Blueprint asset) since the tooling available in this project has no UMG
 * widget-tree authoring API.
 */
UCLASS()
class JINZZA_API UjinzzaMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void OnHostClicked();

	UFUNCTION()
	void OnJoinClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnQuitClicked();

private:
	void HandleSessionStatusChanged(EJinzzaSessionStatus Status, const FString& Message);
	UjinzzaGameInstance* GetJinzzaGameInstance() const;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	FDelegateHandle SessionStatusHandle;
};
