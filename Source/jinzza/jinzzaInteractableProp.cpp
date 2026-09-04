// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractableProp.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

AjinzzaInteractableProp::AjinzzaInteractableProp()
{
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
}

void AjinzzaInteractableProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaInteractableProp, HoldingPawn);
}

void AjinzzaInteractableProp::AttachToHolder(APawn* NewHolder)
{
	if (!HasAuthority() || !NewHolder || InteractionType != EJinzzaPropInteractionType::Handheld || IsHeld())
	{
		return;
	}

	HoldingPawn = NewHolder;
	OnRep_HoldingPawn();
}

void AjinzzaInteractableProp::OnRep_HoldingPawn()
{
	if (ACharacter* HolderCharacter = Cast<ACharacter>(HoldingPawn))
	{
		AttachToComponent(HolderCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, HoldSocketName);
		SetActorEnableCollision(false);
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorEnableCollision(true);
	}
}

void AjinzzaInteractableProp::Activate()
{
	if (!HasAuthority())
	{
		return;
	}

	OnPropActivated();
	Multicast_PlayEffects();
}

void AjinzzaInteractableProp::Multicast_PlayEffects_Implementation()
{
	if (UseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, UseSound, GetActorLocation());
	}

	BP_OnPlayUseEffects();
}
