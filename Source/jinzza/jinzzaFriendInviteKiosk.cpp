// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaFriendInviteKiosk.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "jinzzaFriendInviteWidget.h"
#include "jinzzaUIStyle.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AjinzzaFriendInviteKiosk::AjinzzaFriendInviteKiosk()
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
	Label->SetText(FText::FromString(TEXT("INVITE FRIENDS")));
	Label->SetTextRenderColor(JinzzaUI::Color_Accent.ToFColor(false));

	static ConstructorHelpers::FClassFinder<UjinzzaFriendInviteWidget> FriendInviteWidgetBPClass(TEXT("/Game/JINZZA/UI/Widgets/WBP_FriendInvite"));
	if (FriendInviteWidgetBPClass.Succeeded())
	{
		FriendInviteWidgetClass = FriendInviteWidgetBPClass.Class;
	}
}

void AjinzzaFriendInviteKiosk::Interact(APlayerController* Interactor)
{
	if (!Interactor || !Interactor->IsLocalController())
	{
		return;
	}

	if (ActiveWidget)
	{
		return;
	}

	TSubclassOf<UjinzzaFriendInviteWidget> WidgetClass = FriendInviteWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UjinzzaFriendInviteWidget::StaticClass();
	}

	ActiveWidget = CreateWidget<UjinzzaFriendInviteWidget>(Interactor, WidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport(10);

		TWeakObjectPtr<AjinzzaFriendInviteKiosk> WeakThis(this);
		TWeakObjectPtr<APlayerController> WeakInteractor(Interactor);
		ActiveWidget->OnNativeDestruct.AddLambda([WeakThis, WeakInteractor](UUserWidget*)
		{
			if (AjinzzaFriendInviteKiosk* Kiosk = WeakThis.Get())
			{
				Kiosk->ActiveWidget = nullptr;
			}
			AjinzzaFriendInviteKiosk::ExitKioskUIMode(WeakInteractor.Get());
		});
		AjinzzaFriendInviteKiosk::EnterKioskUIMode(Interactor, ActiveWidget);
	}
}
