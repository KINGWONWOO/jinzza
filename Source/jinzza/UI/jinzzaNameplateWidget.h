// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaNameplateWidget.generated.h"

class UTextBlock;
class APawn;

/**
 * Name tag over a player's head (AjinzzaCharacter's screen-space Nameplate widget component).
 *
 * Shows AjinzzaPartyPlayerState::GetDisplayName(): the nickname in the lobby; in the match the alias
 * ("User1".."UserN" for every candidate, "Judge" for the Judge) - and nothing at all in the match until
 * aliases are handed out, so real names never show there. Hidden on your own character and beyond
 * NameplateMaxDistance. Refreshed every frame (cheap: one string compare) so it follows alias changes
 * and late PlayerState replication without any delegate wiring.
 */
UCLASS()
class JINZZA_API UjinzzaNameplateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	/** Re-evaluates name / visibility. Called every frame by the owning AjinzzaCharacter (a collapsed widget
	 * doesn't tick, so the widget can't do this from its own NativeTick). */
	void Refresh();

	void SetOwnerPawn(APawn* InPawn) { OwnerPawn = InPawn; }

private:
	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<UTextBlock> NameText;

	TWeakObjectPtr<APawn> OwnerPawn;
	FString ShownName;
};
