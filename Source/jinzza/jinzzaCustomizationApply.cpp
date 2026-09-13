// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCustomizationApply.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaCustomizationTypes.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	// Matches MI_Face_Master's expected scalar param (see UjinzzaDisguiseComponent, which has its
	// OWN identically-valued "FaceIndexParamName" in its own anonymous namespace - deliberately
	// named differently here so the two don't collide if unity build ever groups both .cpp files
	// into one translation unit, see [[unreal-mcp-gotchas]] #8). 0 = no override, 1/2/3 = Style
	// A/B/C. EJinzzaCustomizationStyle starts at StyleA=0, so shift by 1.
	const FName CustomizationFaceIndexParamName(TEXT("FaceIndex"));

	// M_Hair_Master's single color param (see [[customization-test-system]]) - reused for the
	// accessory placeholder mesh too, since it's the same "one Vector param" master material.
	const FName HairColorParamName(TEXT("HairColor"));

	const FName HeadSocketName(TEXT("Head"));

	const TCHAR* FaceMasterMaterialPath = TEXT("/Game/JINZZA/Characters/CustomizationTest/M_Face_Master.M_Face_Master");
	const TCHAR* HairMeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* HairMasterMaterialPath = TEXT("/Game/JINZZA/Characters/CustomizationTest/M_Hair_Master.M_Hair_Master");
	const TCHAR* AccessoryMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");

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

	// Placeholder swatch per accessory style - same "no real item art yet" convention as hair
	// color. Matches UjinzzaCustomizationWidget - no swatch shown there yet, text-only row.
	FLinearColor GetAccessoryColorValue(EJinzzaAccessoryStyle Style)
	{
		switch (Style)
		{
		case EJinzzaAccessoryStyle::StyleA: return FLinearColor(0.85f, 0.65f, 0.15f); // gold
		case EJinzzaAccessoryStyle::StyleB: return FLinearColor(0.75f, 0.75f, 0.80f); // silver
		case EJinzzaAccessoryStyle::StyleC: return FLinearColor(0.10f, 0.80f, 0.70f); // teal
		default:                            return FLinearColor::White;
		}
	}
}

void JinzzaCustomization::ApplyToMesh(
	USkeletalMeshComponent* Mesh,
	AActor* MeshOwner,
	TObjectPtr<UMaterialInstanceDynamic>& FaceMaterial,
	TObjectPtr<UStaticMeshComponent>& HairMesh,
	TObjectPtr<UMaterialInstanceDynamic>& HairMaterial,
	TObjectPtr<UStaticMeshComponent>& AccessoryMesh,
	TObjectPtr<UMaterialInstanceDynamic>& AccessoryMaterial,
	const UjinzzaGameUserSettings& Settings)
{
	if (!Mesh || !MeshOwner)
	{
		return;
	}

	// Head style -> FaceIndex, same slot-0/dynamic-material-instance contract
	// UjinzzaDisguiseComponent uses - a round disguise assigned later just overwrites this base
	// FaceIndex value, which is the intended precedence. Loads M_Face_Master directly rather than
	// assuming Mesh->GetMaterial(0) already is it, so this works on any mesh (the real character
	// still needs it as an editor-set override for its OWN non-customization rendering, but this
	// call no longer depends on that being pre-configured).
	if (!FaceMaterial)
	{
		UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, FaceMasterMaterialPath);
		if (!BaseMaterial)
		{
			BaseMaterial = Mesh->GetMaterial(0); // fallback if the placeholder asset path ever moves
		}
		if (BaseMaterial)
		{
			FaceMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, MeshOwner);
			Mesh->SetMaterial(0, FaceMaterial);
		}
	}
	if (FaceMaterial)
	{
		FaceMaterial->SetScalarParameterValue(CustomizationFaceIndexParamName, static_cast<float>(Settings.GetHeadStyle()) + 1.f);
	}

	// Hair color -> a lazily-created placeholder hair mesh socketed to the skeleton's Head
	// socket. Harmlessly skipped if the mesh has no such socket (no crash, just no hair).
	if (!HairMesh && Mesh->DoesSocketExist(HeadSocketName))
	{
		UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, HairMeshPath);
		UMaterialInterface* HairMaster = LoadObject<UMaterialInterface>(nullptr, HairMasterMaterialPath);
		if (SphereMesh && HairMaster)
		{
			HairMesh = NewObject<UStaticMeshComponent>(MeshOwner, TEXT("CustomizationHairMesh"));
			HairMesh->SetStaticMesh(SphereMesh);
			HairMesh->SetRelativeLocation(FVector(-1.f, 0.f, 4.f));
			HairMesh->SetRelativeScale3D(FVector(0.24f, 0.24f, 0.22f));
			HairMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HairMesh->RegisterComponent();
			HairMesh->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform, HeadSocketName);

			HairMaterial = UMaterialInstanceDynamic::Create(HairMaster, MeshOwner);
			HairMesh->SetMaterial(0, HairMaterial);
		}
	}
	if (HairMaterial)
	{
		HairMaterial->SetVectorParameterValue(HairColorParamName, GetHairColorValue(Settings.GetHairColor()));
	}

	// Accessory -> a second placeholder mesh on the same socket, offset clear of the hair sphere.
	// Hidden entirely when AccessoryStyle is None (there's no real accessory content yet either).
	const EJinzzaAccessoryStyle AccessoryStyle = Settings.GetAccessoryStyle();
	if (!AccessoryMesh && Mesh->DoesSocketExist(HeadSocketName))
	{
		UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, AccessoryMeshPath);
		UMaterialInterface* AccessoryMaster = LoadObject<UMaterialInterface>(nullptr, HairMasterMaterialPath);
		if (CubeMesh && AccessoryMaster)
		{
			AccessoryMesh = NewObject<UStaticMeshComponent>(MeshOwner, TEXT("CustomizationAccessoryMesh"));
			AccessoryMesh->SetStaticMesh(CubeMesh);
			AccessoryMesh->SetRelativeLocation(FVector(2.f, 0.f, 6.f));
			AccessoryMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 0.12f));
			AccessoryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			AccessoryMesh->RegisterComponent();
			AccessoryMesh->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform, HeadSocketName);

			AccessoryMaterial = UMaterialInstanceDynamic::Create(AccessoryMaster, MeshOwner);
			AccessoryMesh->SetMaterial(0, AccessoryMaterial);
		}
	}
	if (AccessoryMesh)
	{
		AccessoryMesh->SetVisibility(AccessoryStyle != EJinzzaAccessoryStyle::None);
	}
	if (AccessoryMaterial)
	{
		AccessoryMaterial->SetVectorParameterValue(HairColorParamName, GetAccessoryColorValue(AccessoryStyle));
	}

	// Top/Eyebrows/Eyes: still no matching mesh/material content - left as a no-op, same as before.
}
