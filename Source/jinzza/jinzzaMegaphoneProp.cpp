// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMegaphoneProp.h"
#include "Net/UnrealNetwork.h"

void AjinzzaMegaphoneProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaMegaphoneProp, bIsAmplifying);
}

void AjinzzaMegaphoneProp::OnPropActivated_Implementation()
{
	bIsAmplifying = !bIsAmplifying;
	OnRep_IsAmplifying();
}

void AjinzzaMegaphoneProp::OnRep_IsAmplifying()
{
	BP_OnAmplifyStateChanged(bIsAmplifying);
}
