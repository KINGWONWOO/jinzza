// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaTestDrillKiosk.h"
#include "jinzzaTestPlayerController.h"
#include "jinzzaWorldSignWidget.h"
#include "jinzzaUIStyle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FLinearColor DrillColor(EJinzzaTestDrill Drill)
	{
		switch (Drill)
		{
		case EJinzzaTestDrill::SelfIntroduction: return JinzzaUI::Sticker_Yellow;
		case EJinzzaTestDrill::QuestionAsJudge: return JinzzaUI::Sticker_Sky;
		case EJinzzaTestDrill::QuestionAsCandidate: return JinzzaUI::Sticker_Teal;
		case EJinzzaTestDrill::Vote: return JinzzaUI::Sticker_Pink;
		default: return JinzzaUI::Sticker_Coral;
		}
	}
}

AjinzzaTestDrillKiosk::AjinzzaTestDrillKiosk()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	InteractionRadius = 160.f;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// 50 cm wide, 90 cm tall pedestal standing on the actor's origin (the floor).
	Pedestal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pedestal"));
	Pedestal->SetupAttachment(Root);
	Pedestal->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	Pedestal->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.9f));
	Pedestal->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Pedestal->SetCollisionResponseToAllChannels(ECR_Block);
	if (CylinderFinder.Succeeded())
	{
		Pedestal->SetStaticMesh(CylinderFinder.Object);
	}

	// The big colored button on top.
	Button = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Button"));
	Button->SetupAttachment(Root);
	Button->SetRelativeLocation(FVector(0.f, 0.f, 98.f));
	Button->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.16f));
	Button->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (SphereFinder.Succeeded())
	{
		Button->SetStaticMesh(SphereFinder.Object);
	}

	Label = CreateDefaultSubobject<UWidgetComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 175.f));
	Label->SetRelativeScale3D(FVector(0.4f));
	Label->SetWidgetSpace(EWidgetSpace::World);
	Label->SetDrawSize(FVector2D(520.f, 150.f));
	Label->SetPivot(FVector2D(0.5f, 0.5f));
	Label->SetTwoSided(true);
	Label->SetBlendMode(EWidgetBlendMode::Transparent);
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetWidgetClass(UjinzzaWorldSignWidget::StaticClass());
}

FText AjinzzaTestDrillKiosk::GetTitleText() const
{
	if (!Title.IsEmpty())
	{
		return Title;
	}
	switch (Drill)
	{
	case EJinzzaTestDrill::SelfIntroduction: return FText::FromString(TEXT("자기소개 체험\nSelf-introduction"));
	case EJinzzaTestDrill::QuestionAsJudge: return FText::FromString(TEXT("질문 타임 - 판정단\nQuestion Time (Judge)"));
	case EJinzzaTestDrill::QuestionAsCandidate: return FText::FromString(TEXT("질문 타임 - 후보\nQuestion Time (Candidate)"));
	case EJinzzaTestDrill::Vote: return FText::FromString(TEXT("투표 체험\nJudge Vote"));
	default: return FText::FromString(TEXT("초기화\nReset"));
	}
}

FText AjinzzaTestDrillKiosk::GetInteractionPrompt() const
{
	switch (Drill)
	{
	case EJinzzaTestDrill::SelfIntroduction: return FText::FromString(TEXT("Start: Self-introduction"));
	case EJinzzaTestDrill::QuestionAsJudge: return FText::FromString(TEXT("Start: Question Time as Judge"));
	case EJinzzaTestDrill::QuestionAsCandidate: return FText::FromString(TEXT("Start: Question Time as Candidate"));
	case EJinzzaTestDrill::Vote: return FText::FromString(TEXT("Start: Judge Vote"));
	default: return FText::FromString(TEXT("Reset the drills"));
	}
}

void AjinzzaTestDrillKiosk::Interact(APlayerController* Interactor)
{
	if (AjinzzaTestPlayerController* TestPC = Cast<AjinzzaTestPlayerController>(Interactor); TestPC && TestPC->IsLocalController())
	{
		TestPC->Server_StartDrill(Drill);
	}
}

void AjinzzaTestDrillKiosk::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyLook();
}

void AjinzzaTestDrillKiosk::BeginPlay()
{
	Super::BeginPlay();
	ApplyLook();
}

void AjinzzaTestDrillKiosk::ApplyLook()
{
	// BasicShapeMaterial's "Color" parameter: dark pedestal, drill-colored button.
	if (Pedestal && Pedestal->GetStaticMesh())
	{
		if (UMaterialInstanceDynamic* Material = Pedestal->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.05f, 0.06f));
		}
	}
	if (Button && Button->GetStaticMesh())
	{
		if (UMaterialInstanceDynamic* Material = Button->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), DrillColor(Drill));
		}
	}

	if (Label)
	{
		if (!Label->GetUserWidgetObject())
		{
			Label->InitWidget();
		}
		if (UjinzzaWorldSignWidget* Sign = Cast<UjinzzaWorldSignWidget>(Label->GetUserWidgetObject()))
		{
			Sign->SetSignText(GetTitleText());
		}
	}
}
