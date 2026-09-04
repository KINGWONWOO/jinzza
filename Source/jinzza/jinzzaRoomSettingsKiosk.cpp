// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaRoomSettingsKiosk.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaRoomSettingsWidget.h"
#include "jinzzaUIStyle.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AjinzzaRoomSettingsKiosk::AjinzzaRoomSettingsKiosk()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 1.6f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMeshFinder.Object);
	}

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(RootComponent);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	// TextRenderComponent's quad only reads correctly from its front face (local +X rotated 180 here so it
	// faces -X); the kiosk sits at the center of a circular room with spawns on all sides, so no single
	// orientation reads right from every seat - this just picks the side most players naturally approach from.
	Label->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(28.f);
	Label->SetText(FText::FromString(TEXT("ROOM SETTINGS")));
	Label->SetTextRenderColor(JinzzaUI::Color_Accent.ToFColor(false));
}

void AjinzzaRoomSettingsKiosk::Interact(APlayerController* Interactor)
{
	if (!Interactor || !Interactor->IsLocalController())
	{
		return;
	}

	if (ActiveWidget)
	{
		return;
	}

	TSubclassOf<UjinzzaRoomSettingsWidget> WidgetClass = RoomSettingsWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UjinzzaRoomSettingsWidget::StaticClass();
	}

	ActiveWidget = CreateWidget<UjinzzaRoomSettingsWidget>(Interactor, WidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport(10);

		TWeakObjectPtr<AjinzzaRoomSettingsKiosk> WeakThis(this);
		ActiveWidget->OnNativeDestruct.AddLambda([WeakThis](UUserWidget*)
		{
			if (AjinzzaRoomSettingsKiosk* Kiosk = WeakThis.Get())
			{
				Kiosk->ActiveWidget = nullptr;
			}
		});
		Interactor->bShowMouseCursor = true;
	}
}
