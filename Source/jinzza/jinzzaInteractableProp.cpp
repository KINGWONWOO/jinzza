// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractableProp.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "jinzzaAudio.h"
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
	// Sized to the widget itself - the key cap is as wide as the bound key's name ("E" vs "Space Bar").
	InteractionPromptComponent->SetDrawAtDesiredSize(true);
	InteractionPromptComponent->SetVisibility(false);

	// TEMP placeholder audio (a dull wooden knock) - swap for real per-prop impact sounds later.
	static ConstructorHelpers::FObjectFinder<USoundBase> ImpactSoundFinder(TEXT("/Game/JINZZA/Audio/Sounds/Basketball/InteractWoodenBox__cut_0sec_.InteractWoodenBox__cut_0sec_"));
	if (ImpactSoundFinder.Succeeded())
	{
		ImpactSound = ImpactSoundFinder.Object;
	}
}

void AjinzzaInteractableProp::BeginPlay()
{
	Super::BeginPlay();

	// Impacts are detected by the server's physics only (clients see replicated movement, not their own sim)
	// and then broadcast, so everyone in range hears the same knock at the same spot.
	if (HasAuthority() && ImpactSound)
	{
		Mesh->SetNotifyRigidBodyCollision(true);
		Mesh->OnComponentHit.AddDynamic(this, &AjinzzaInteractableProp::OnMeshHit);
	}

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
	// The interact key may have been rebound since this prompt was last on screen.
	if (UjinzzaInteractionPromptWidget* PromptWidget = Cast<UjinzzaInteractionPromptWidget>(InteractionPromptComponent->GetUserWidgetObject()))
	{
		PromptWidget->RefreshBoundKey();
	}
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

	// Whoever was holding the use button on it (if anyone) no longer is.
	EndHoldUse();

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

	EndHoldUse();

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

	EndHoldUse();

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

void AjinzzaInteractableProp::BeginHoldUse()
{
	if (!HasAuthority() || HoldRepeatInterval <= 0.f || !IsHeld())
	{
		return;
	}

	// The button press that got us here already did one immediate Activate(), so the first repeat is one
	// full interval away (SetTimer's default first delay is the rate itself).
	GetWorldTimerManager().SetTimer(HoldUseTimerHandle, this, &AjinzzaInteractableProp::Activate, HoldRepeatInterval, true);
}

void AjinzzaInteractableProp::EndHoldUse()
{
	GetWorldTimerManager().ClearTimer(HoldUseTimerHandle);
}

void AjinzzaInteractableProp::OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only free-flying props knock - a held prop has physics off, and a resting one reports no new hits.
	if (!Mesh->IsSimulatingPhysics())
	{
		return;
	}

	// Rolling/sliding keeps firing tiny hits every physics step; only a real knock (and not more often than
	// ImpactCooldown) makes a sound. NormalImpulse / mass = the speed change the hit caused (cm/s).
	const float Mass = FMath::Max(Mesh->GetMass(), KINDA_SMALL_NUMBER);
	const float ImpactSpeed = NormalImpulse.Size() / Mass;
	const float Now = GetWorld()->GetTimeSeconds();
	if (ImpactSpeed < MinImpactSpeed || Now - LastImpactTime < ImpactCooldown)
	{
		return;
	}
	LastImpactTime = Now;

	const float Volume = FMath::GetMappedRangeValueClamped(FVector2f(MinImpactSpeed, LoudImpactSpeed), FVector2f(0.3f, 1.f), ImpactSpeed);
	Multicast_PlayImpact(Hit.ImpactPoint, Volume);
}

void AjinzzaInteractableProp::Multicast_PlayImpact_Implementation(FVector_NetQuantize Location, float Volume)
{
	JinzzaAudio::PlaySoundAt(this, ImpactSound, Location, ImpactAudibleRadius, Volume);
}

void AjinzzaInteractableProp::Multicast_PlayEffects_Implementation()
{
	if (UseSound)
	{
		JinzzaAudio::PlaySoundAt(this, UseSound, GetActorLocation(), NoiseRadius);
	}

	BP_OnPlayUseEffects();
}
