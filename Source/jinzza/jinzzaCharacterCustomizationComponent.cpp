// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCharacterCustomizationComponent.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaCustomizationApply.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

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

	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	const ACharacter* OwningCharacter = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = OwningCharacter ? OwningCharacter->GetMesh() : nullptr;
	if (!Mesh)
	{
		return;
	}

	JinzzaCustomization::ApplyToMesh(Mesh, GetOwner(), DynamicFaceMaterial, HairMeshComponent,
		DynamicHairMaterial, AccessoryMeshComponent, DynamicAccessoryMaterial, *Settings);
}
