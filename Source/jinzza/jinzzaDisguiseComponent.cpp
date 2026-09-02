// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaDisguiseComponent.h"
#include "jinzzaPartyPlayerState.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	// Matches MI_Face_Master's expected scalar param, once that content exists (design doc
	// section 13-2). 0 = no override (use the material's default / no disguise).
	const FName FaceIndexParamName(TEXT("FaceIndex"));
}

UjinzzaDisguiseComponent::UjinzzaDisguiseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UjinzzaDisguiseComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshDisguise();
}

void UjinzzaDisguiseComponent::RefreshDisguise()
{
	const ACharacter* OwningCharacter = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = OwningCharacter ? OwningCharacter->GetMesh() : nullptr;
	if (!Mesh)
	{
		return;
	}

	const AjinzzaPartyPlayerState* PartyPS = OwningCharacter->GetPlayerState<AjinzzaPartyPlayerState>();
	const EJinzzaFaceType FaceType = (PartyPS && !PartyPS->IsGhost()) ? PartyPS->GetFaceType() : EJinzzaFaceType::None;

	if (FaceType == EJinzzaFaceType::None)
	{
		return;
	}

	if (!DynamicFaceMaterial)
	{
		UMaterialInterface* BaseMaterial = Mesh->GetMaterial(0);
		if (!BaseMaterial)
		{
			return;
		}
		DynamicFaceMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Mesh->SetMaterial(0, DynamicFaceMaterial);
	}

	DynamicFaceMaterial->SetScalarParameterValue(FaceIndexParamName, static_cast<float>(FaceType));
}
