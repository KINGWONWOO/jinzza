// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMenuBackgroundCharacter.h"

AjinzzaMenuBackgroundCharacter::AjinzzaMenuBackgroundCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AutoPossessAI = EAutoPossessAI::Disabled;
}
