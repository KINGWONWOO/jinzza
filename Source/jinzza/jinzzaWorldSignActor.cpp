// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaWorldSignActor.h"
#include "Components/WidgetComponent.h"
#include "jinzzaWorldSignWidget.h"

AjinzzaWorldSignActor::AjinzzaWorldSignActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SignWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SignWidgetComponent"));
	RootComponent = SignWidgetComponent;
	SignWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	SignWidgetComponent->SetDrawSize(SignSize);
	SignWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	SignWidgetComponent->SetTwoSided(false);
	SignWidgetComponent->SetBlendMode(EWidgetBlendMode::Transparent);
}

void AjinzzaWorldSignActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySign();
}

#if WITH_EDITOR
void AjinzzaWorldSignActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplySign();
}
#endif

void AjinzzaWorldSignActor::BeginPlay()
{
	Super::BeginPlay();
	ApplySign();
}

void AjinzzaWorldSignActor::ApplySign()
{
	if (!SignWidgetComponent)
	{
		return;
	}

	SignWidgetComponent->SetDrawSize(SignSize);

	if (SignWidgetClass && SignWidgetComponent->GetWidgetClass() != SignWidgetClass)
	{
		SignWidgetComponent->SetWidgetClass(SignWidgetClass);
	}

	if (!SignWidgetComponent->GetUserWidgetObject())
	{
		SignWidgetComponent->InitWidget();
	}

	if (UjinzzaWorldSignWidget* Widget = Cast<UjinzzaWorldSignWidget>(SignWidgetComponent->GetUserWidgetObject()))
	{
		Widget->SetSignText(SignText);
	}
}
