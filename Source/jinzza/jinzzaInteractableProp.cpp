// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractableProp.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "jinzzaCharacter.h"
#include "jinzzaInteractionPromptWidget.h"

AjinzzaInteractableProp::AjinzzaInteractableProp()
{
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	InteractionPromptComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptComponent"));
	InteractionPromptComponent->SetupAttachment(RootComponent);
	InteractionPromptComponent->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	InteractionPromptComponent->SetWidgetSpace(EWidgetSpace::Screen);
	InteractionPromptComponent->SetDrawSize(FVector2D(150.f, 60.f));
	InteractionPromptComponent->SetVisibility(false);
}

void AjinzzaInteractableProp::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionPromptWidgetClass)
	{
		InteractionPromptComponent->SetWidgetClass(InteractionPromptWidgetClass);
		if (UjinzzaInteractionPromptWidget* PromptWidget = Cast<UjinzzaInteractionPromptWidget>(InteractionPromptComponent->GetUserWidgetObject()))
		{
			PromptWidget->SetPrompt(InteractPromptText);
		}
	}
}

void AjinzzaInteractableProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaInteractableProp, HoldingPawn);
}

void AjinzzaInteractableProp::ShowInteractionPrompt()
{
	InteractionPromptComponent->SetVisibility(true);
}

void AjinzzaInteractableProp::HideInteractionPrompt()
{
	InteractionPromptComponent->SetVisibility(false);
}

void AjinzzaInteractableProp::AttachToHolder(APawn* NewHolder)
{
	if (!HasAuthority() || !NewHolder || InteractionType != EJinzzaPropInteractionType::Handheld || HoldingPawn == NewHolder)
	{
		return;
	}

	// Snatching: whoever holds this already (if anyone) loses it - tell them to forget it before
	// reassigning, so their own HeldProp pointer doesn't go stale.
	if (AjinzzaCharacter* PreviousHolder = Cast<AjinzzaCharacter>(HoldingPawn))
	{
		PreviousHolder->ClearHeldPropIfMatches(this);
	}

	// Physics must be off before attaching, or the attachment transform and physics sim fight each frame.
	Mesh->SetSimulatePhysics(false);
	APawn* OldHolder = HoldingPawn;
	HoldingPawn = NewHolder;
	OnRep_HoldingPawn(OldHolder);
}

void AjinzzaInteractableProp::DropFromHolder()
{
	if (!HasAuthority() || !IsHeld())
	{
		return;
	}

	APawn* OldHolder = HoldingPawn;
	HoldingPawn = nullptr;
	OnRep_HoldingPawn(OldHolder); // detaches (keeping current world transform) and re-enables collision
	Mesh->SetSimulatePhysics(true); // let it fall/settle from the hand position it was dropped at
}

void AjinzzaInteractableProp::ThrowFromHolder(const FVector& LaunchVelocity)
{
	if (!HasAuthority() || !IsHeld())
	{
		return;
	}

	APawn* OldHolder = HoldingPawn;
	HoldingPawn = nullptr;
	OnRep_HoldingPawn(OldHolder); // detaches (keeping current world transform) and re-enables collision
	Mesh->SetSimulatePhysics(true);
	Mesh->SetPhysicsLinearVelocity(LaunchVelocity);
}

void AjinzzaInteractableProp::OnRep_HoldingPawn(APawn* OldHoldingPawn)
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

	// Local-only HUD cosmetics: every client runs this (both from real replication on remote
	// clients and the server's own manual calls above), but only the locally-controlled
	// character actually gaining/losing this prop reacts - everyone else is a no-op.
	if (AjinzzaCharacter* NewCharacter = Cast<AjinzzaCharacter>(HoldingPawn))
	{
		if (NewCharacter->IsLocallyControlled())
		{
			NewCharacter->ShowPropUsageHUD(this);
		}
	}
	if (AjinzzaCharacter* OldCharacter = Cast<AjinzzaCharacter>(OldHoldingPawn))
	{
		if (OldCharacter->IsLocallyControlled())
		{
			OldCharacter->HidePropUsageHUD(this);
		}
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
