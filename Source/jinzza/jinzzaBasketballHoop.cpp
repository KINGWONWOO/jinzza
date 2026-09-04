// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaBasketballHoop.h"
#include "jinzzaBasketballProp.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AjinzzaBasketballHoop::AjinzzaBasketballHoop()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	HoopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HoopMesh"));
	RootComponent = HoopMesh;

	ScoringVolume = CreateDefaultSubobject<USphereComponent>(TEXT("ScoringVolume"));
	ScoringVolume->SetupAttachment(RootComponent);
	ScoringVolume->SetSphereRadius(25.f);
	ScoringVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ScoringVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void AjinzzaBasketballHoop::BeginPlay()
{
	Super::BeginPlay();

	// Server-only: each client's own overlap event would otherwise fire this independently.
	if (HasAuthority())
	{
		ScoringVolume->OnComponentBeginOverlap.AddDynamic(this, &AjinzzaBasketballHoop::OnScoringVolumeOverlap);
	}
}

void AjinzzaBasketballHoop::OnScoringVolumeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bOnCooldown || !Cast<AjinzzaBasketballProp>(OtherActor))
	{
		return;
	}

	bOnCooldown = true;
	GetWorldTimerManager().SetTimer(CooldownTimerHandle, this, &AjinzzaBasketballHoop::ClearScoreCooldown, ScoreCooldown, false);

	Multicast_PlayScoreEffects();
}

void AjinzzaBasketballHoop::ClearScoreCooldown()
{
	bOnCooldown = false;
}

void AjinzzaBasketballHoop::Multicast_PlayScoreEffects_Implementation()
{
	if (ScoreSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ScoreSound, GetActorLocation());
	}

	BP_OnPlayScoreEffects();
}
