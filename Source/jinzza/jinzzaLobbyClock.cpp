// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyClock.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "jinzzaUIStyle.h"
#include "jinzzaLobbyGameState.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Deliberately not literal noon/midnight (which would leave the hour hand indistinguishable
	// between Day and Night on a 12-hour face) - these are picked purely so all three presets read
	// as visually distinct at a glance. Minute hand always stays at :00.
	//
	// Negated: positive Roll sweeps counterclockwise as seen from the room (confirmed visually -
	// Day's un-negated 270 rendered pointing at 3:00 instead of 9:00), so the sign is flipped here
	// to make the hour-hand position actually match its comment.
	float GetHourHandRollDegrees(EJinzzaLobbyTimeOfDay TimeOfDay)
	{
		switch (TimeOfDay)
		{
		case EJinzzaLobbyTimeOfDay::Day:    return -(9.f / 12.f * 360.f);  // 9:00
		case EJinzzaLobbyTimeOfDay::Sunset: return -(18.f / 12.f * 360.f); // 6:00 face position
		case EJinzzaLobbyTimeOfDay::Night:  return -(23.f / 12.f * 360.f); // 11:00 face position
		default:                            return 0.f;
		}
	}
}

AjinzzaLobbyClock::AjinzzaLobbyClock()
{
	PrimaryActorTick.bCanEverTick = false;

	ClockRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ClockRoot"));
	RootComponent = ClockRoot;

	// Flattened Cylinder: radius via X/Y scale, thin along its own Z (height) axis, which the
	// Pitch=90 rotation below points along the actor's local X - i.e. straight out of the wall.
	Face = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Face"));
	Face->SetupAttachment(RootComponent);
	Face->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	Face->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.02f));
	Face->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Face->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		Face->SetStaticMesh(CylinderMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// Rest pose (pivot Roll=0) points straight up local +Z, i.e. 12 o'clock; the hand mesh's
	// RelativeLocation.Z is half its own length so it spans from the pivot (the clock's center)
	// outward, one end at the center - see the HourPivot/MinutePivot comment in the header for
	// why the mesh itself is never rotated directly.
	HourPivot = CreateDefaultSubobject<USceneComponent>(TEXT("HourPivot"));
	HourPivot->SetupAttachment(RootComponent);

	HourHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HourHand"));
	HourHand->SetupAttachment(HourPivot);
	HourHand->SetRelativeLocation(FVector(4.f, 0.f, 7.5f));
	HourHand->SetRelativeScale3D(FVector(0.01f, 0.03f, 0.15f));
	HourHand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CubeMeshFinder.Succeeded())
	{
		HourHand->SetStaticMesh(CubeMeshFinder.Object);
	}

	MinutePivot = CreateDefaultSubobject<USceneComponent>(TEXT("MinutePivot"));
	MinutePivot->SetupAttachment(RootComponent);

	MinuteHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MinuteHand"));
	MinuteHand->SetupAttachment(MinutePivot);
	MinuteHand->SetRelativeLocation(FVector(4.f, 0.f, 9.5f));
	MinuteHand->SetRelativeScale3D(FVector(0.01f, 0.02f, 0.19f));
	MinuteHand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CubeMeshFinder.Succeeded())
	{
		MinuteHand->SetStaticMesh(CubeMeshFinder.Object);
	}

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(RootComponent);
	Label->SetRelativeLocation(FVector(4.f, 0.f, -26.f));
	Label->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(10.f);
	Label->SetText(FText::FromString(TEXT("CLOCK")));
	Label->SetTextRenderColor(JinzzaUI::Color_Accent.ToFColor(false));

	SetDisplayedTime(EJinzzaLobbyTimeOfDay::Day);
}

void AjinzzaLobbyClock::Interact(APlayerController* Interactor)
{
	if (!Interactor || !Interactor->IsLocalController() || !Interactor->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	AjinzzaLobbyGameState* LobbyGameState = World ? World->GetGameState<AjinzzaLobbyGameState>() : nullptr;
	if (LobbyGameState)
	{
		LobbyGameState->CycleTimeOfDay();
	}
}

void AjinzzaLobbyClock::SetDisplayedTime(EJinzzaLobbyTimeOfDay NewTimeOfDay)
{
	if (HourPivot)
	{
		HourPivot->SetRelativeRotation(FRotator(0.f, 0.f, GetHourHandRollDegrees(NewTimeOfDay)));
	}

	if (MinutePivot)
	{
		MinutePivot->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	}
}
