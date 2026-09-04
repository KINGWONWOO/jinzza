// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaWardrobeKiosk.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "jinzzaCustomizationWidget.h"
#include "jinzzaUIStyle.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AjinzzaWardrobeKiosk::AjinzzaWardrobeKiosk()
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
	Label->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(28.f);
	Label->SetText(FText::FromString(TEXT("WARDROBE")));
	Label->SetTextRenderColor(JinzzaUI::Color_Accent.ToFColor(false));

	static ConstructorHelpers::FClassFinder<UjinzzaCustomizationWidget> CustomizationWidgetBPClass(TEXT("/Game/JINZZA/UI/Widgets/WBP_Customization"));
	if (CustomizationWidgetBPClass.Succeeded())
	{
		CustomizationWidgetClass = CustomizationWidgetBPClass.Class;
	}
}

void AjinzzaWardrobeKiosk::Interact(APlayerController* Interactor)
{
	if (!Interactor || !Interactor->IsLocalController())
	{
		return;
	}

	if (ActiveWidget)
	{
		return;
	}

	TSubclassOf<UjinzzaCustomizationWidget> WidgetClass = CustomizationWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UjinzzaCustomizationWidget::StaticClass();
	}

	ActiveWidget = CreateWidget<UjinzzaCustomizationWidget>(Interactor, WidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport(10);

		TWeakObjectPtr<AjinzzaWardrobeKiosk> WeakThis(this);
		ActiveWidget->OnBackRequested.AddLambda([WeakThis]()
		{
			if (AjinzzaWardrobeKiosk* Kiosk = WeakThis.Get())
			{
				if (Kiosk->ActiveWidget)
				{
					Kiosk->ActiveWidget->RemoveFromParent();
					Kiosk->ActiveWidget = nullptr;
				}
			}
		});
		Interactor->bShowMouseCursor = true;
	}
}
