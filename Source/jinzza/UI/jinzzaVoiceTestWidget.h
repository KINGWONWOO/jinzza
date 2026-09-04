// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaRoundTypes.h"
#include "jinzzaVoiceTestWidget.generated.h"

class UButton;
class UTextBlock;
class UAudioCaptureComponent;

/**
 * Bottom-right main menu panel: capture the mic (UAudioCaptureComponent, real-time, local
 * loopback only - no networking) and play it back live through EJinzzaVoiceFilter's presets, so
 * the player can hear what a disguised voice would sound like before the real proximity-voice
 * system (Vivox, design doc section 11, Week 6) exists. None/High/Low are a real, live
 * UAudioComponent::SetPitchMultiplier. Robot is currently a labeled no-op (neutral pitch, same
 * as None) - a real ring-modulator needs the Synthesis plugin module, which isn't a
 * jinzza.Build.cs dependency yet; adding one forces a full UnrealBuildTool rebuild (the editor
 * fully closed), not something to do mid-session - see ApplyCurrentFilter's Robot case.
 *
 * UMG-authored: add a UButton "ToggleListenButton" (with a child UTextBlock
 * "ToggleListenButtonText"), a UTextBlock "FilterText", and UButtons "PrevFilterButton"/
 * "NextFilterButton" to the Widget Blueprint (e.g. WBP_VoiceTest) that subclasses this - all
 * BindWidgetOptional so the class compiles either way.
 */
UCLASS()
class JINZZA_API UjinzzaVoiceTestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void OnToggleListenClicked();

	UFUNCTION()
	void OnNextFilterClicked();

	UFUNCTION()
	void OnPrevFilterClicked();

private:
	void StopListening();
	void ApplyCurrentFilter();
	void RefreshFilterText();
	static FText GetFilterDisplayName(EJinzzaVoiceFilter Filter);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ToggleListenButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ToggleListenButtonText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FilterText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NextFilterButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PrevFilterButton;

	/** Created on first Listen click, outered to the owning PlayerController so it survives
	 * independently of this widget - see NativeDestruct for cleanup. */
	UPROPERTY()
	TObjectPtr<UAudioCaptureComponent> CaptureComponent;

	EJinzzaVoiceFilter CurrentFilter = EJinzzaVoiceFilter::None;
	bool bIsListening = false;
};
