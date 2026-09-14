// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaAuditionCurtain.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AjinzzaAuditionCurtain::AjinzzaAuditionCurtain()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CurtainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CurtainMesh"));
	RootComponent = CurtainMesh;
	CurtainMesh->SetMobility(EComponentMobility::Movable);
}

void AjinzzaAuditionCurtain::BeginPlay()
{
	Super::BeginPlay();
	ClosedRelativeZ = CurtainMesh->GetRelativeLocation().Z;
}

void AjinzzaAuditionCurtain::Open()
{
	if (!HasAuthority())
	{
		return;
	}
	bOpen = true;
}

void AjinzzaAuditionCurtain::Close()
{
	if (!HasAuthority())
	{
		return;
	}
	bOpen = false;
}

void AjinzzaAuditionCurtain::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float TargetZ = ClosedRelativeZ + (bOpen ? RaiseHeight : 0.f);
	FVector Loc = CurtainMesh->GetRelativeLocation();
	if (FMath::IsNearlyEqual(Loc.Z, TargetZ, 0.1f))
	{
		return;
	}

	const float Speed = (TransitionSeconds > 0.f) ? (RaiseHeight / TransitionSeconds) : RaiseHeight;
	Loc.Z = FMath::FInterpConstantTo(Loc.Z, TargetZ, DeltaSeconds, Speed);
	CurtainMesh->SetRelativeLocation(Loc);
}

void AjinzzaAuditionCurtain::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AjinzzaAuditionCurtain, bOpen);
}
