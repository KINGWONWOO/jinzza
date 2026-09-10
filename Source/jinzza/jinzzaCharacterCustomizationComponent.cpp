// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCharacterCustomizationComponent.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaCustomizationTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	// Matches MI_Face_Master's expected scalar param (see UjinzzaDisguiseComponent) - 0 = no
	// override, 1/2/3 = Style A/B/C. EJinzzaCustomizationStyle starts at StyleA=0, so shift by 1.
	const FName FaceIndexParamName(TEXT("FaceIndex"));

	// M_Hair_Master's single color param (see [[customization-test-system]]).
	const FName HairColorParamName(TEXT("HairColor"));

	const FName HeadSocketName(TEXT("Head"));

	const TCHAR* HairMeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* HairMasterMaterialPath = TEXT("/Game/JINZZA/Characters/CustomizationTest/M_Hair_Master.M_Hair_Master");

	// Matches UjinzzaCustomizationWidget::GetHairColorSwatchColor exactly - keep in sync so the
	// menu's swatch always matches what actually gets applied to the character.
	FLinearColor GetHairColorValue(EJinzzaHairColor Color)
	{
		switch (Color)
		{
		case EJinzzaHairColor::Brown:  return FLinearColor(0.36f, 0.20f, 0.09f);
		case EJinzzaHairColor::Blonde: return FLinearColor(0.85f, 0.70f, 0.35f);
		case EJinzzaHairColor::Red:    return FLinearColor(0.55f, 0.11f, 0.06f);
		case EJinzzaHairColor::Blue:   return FLinearColor(0.10f, 0.30f, 0.75f);
		default:                       return FLinearColor(0.03f, 0.03f, 0.03f);
		}
	}
}

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

	// Head style -> FaceIndex, same slot-0/dynamic-material-instance contract
	// UjinzzaDisguiseComponent uses - a round disguise assigned later just overwrites this base
	// FaceIndex value, which is the intended precedence (see that class's comment).
	if (!DynamicFaceMaterial)
	{
		if (UMaterialInterface* BaseMaterial = Mesh->GetMaterial(0))
		{
			DynamicFaceMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Mesh->SetMaterial(0, DynamicFaceMaterial);
		}
	}
	if (DynamicFaceMaterial)
	{
		DynamicFaceMaterial->SetScalarParameterValue(FaceIndexParamName, static_cast<float>(Settings->GetHeadStyle()) + 1.f);
	}

	// Hair color -> a lazily-created placeholder hair mesh socketed to the skeleton's Head
	// socket. Harmlessly skipped if the mesh has no such socket (no crash, just no hair).
	if (!HairMeshComponent && Mesh->DoesSocketExist(HeadSocketName))
	{
		UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, HairMeshPath);
		UMaterialInterface* HairMaster = LoadObject<UMaterialInterface>(nullptr, HairMasterMaterialPath);
		if (SphereMesh && HairMaster)
		{
			HairMeshComponent = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("CustomizationHairMesh"));
			HairMeshComponent->SetStaticMesh(SphereMesh);
			HairMeshComponent->SetRelativeLocation(FVector(-1.f, 0.f, 4.f));
			HairMeshComponent->SetRelativeScale3D(FVector(0.24f, 0.24f, 0.22f));
			HairMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HairMeshComponent->RegisterComponent();
			HairMeshComponent->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform, HeadSocketName);

			DynamicHairMaterial = UMaterialInstanceDynamic::Create(HairMaster, this);
			HairMeshComponent->SetMaterial(0, DynamicHairMaterial);
		}
	}
	if (DynamicHairMaterial)
	{
		DynamicHairMaterial->SetVectorParameterValue(HairColorParamName, GetHairColorValue(Settings->GetHairColor()));
	}

	// Top/Eyebrows/Eyes: still no matching mesh/material content - left as a no-op, same as before.
}
