// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaBoomboxProp.h"
#include "Components/AudioComponent.h"
#include "Net/UnrealNetwork.h"

AjinzzaBoomboxProp::AjinzzaBoomboxProp()
{
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(RootComponent);
	AudioComponent->bAutoActivate = false;
}

void AjinzzaBoomboxProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaBoomboxProp, bIsPlaying);
}

void AjinzzaBoomboxProp::OnPropActivated_Implementation()
{
	bIsPlaying = !bIsPlaying;
	OnRep_IsPlaying();
}

void AjinzzaBoomboxProp::OnRep_IsPlaying()
{
	if (!AudioComponent)
	{
		return;
	}

	if (bIsPlaying && Track)
	{
		AudioComponent->SetSound(Track);
		AudioComponent->Play();
	}
	else
	{
		AudioComponent->Stop();
	}
}
