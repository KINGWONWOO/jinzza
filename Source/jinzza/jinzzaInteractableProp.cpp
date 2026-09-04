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

	// Physics must be off before attaching, or the attachment transform and physics sim fight each frame.
	Mesh->SetSimulatePhysics(false);
	HoldingPawn = NewHolder;
	OnRep_HoldingPawn();
}

void AjinzzaInteractableProp::DropFromHolder()
{
	if (!HasAuthority() || !IsHeld())
	{
		return;
	}

	HoldingPawn = nullptr;
	OnRep_HoldingPawn(); // detaches (keeping current world transform) and re-enables collision
	Mesh->SetSimulatePhysics(true); // let it fall/settle from the hand position it was dropped at
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
