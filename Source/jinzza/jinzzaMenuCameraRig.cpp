// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMenuCameraRig.h"
#include "Camera/CameraComponent.h"

AjinzzaMenuCameraRig::AjinzzaMenuCameraRig()
{
	PrimaryActorTick.bCanEverTick = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	SetRootComponent(CameraComponent);
}
