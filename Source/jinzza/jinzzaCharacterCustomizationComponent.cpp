// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCharacterCustomizationComponent.h"
#include "jinzzaGameUserSettings.h"
#include "GameFramework/Pawn.h"

void UjinzzaCharacterCustomizationComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshCustomization();
}

void UjinzzaCharacterCustomizationComponent::RefreshCustomization()
{
	// UjinzzaGameUserSettings is this client's own local config, not replicated - applying it to
	// anyone but the locally-controlled character would paint every remote pawn with whatever
	// THIS client happens to have selected.
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!OwningPawn || !OwningPawn->IsLocallyControlled())
	{
		return;
	}

	if (!UjinzzaGameUserSettings::Get())
	{
		return;
	}

	// No real head/hair/top/eyebrow/eye meshes or materials exist yet (see
	// jinzzaCustomizationTypes.h) - this is where swapping them in belongs once they do, reading
	// UjinzzaGameUserSettings::Get()->GetHeadStyle()/GetHairColor()/GetTopStyle()/
	// GetEyebrowsStyle()/GetEyesStyle().
}
