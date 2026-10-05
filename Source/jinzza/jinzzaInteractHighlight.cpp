// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInteractHighlight.h"
#include "Components/MeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* OutlineMaterialPath = TEXT("/Game/JINZZA/Materials/PostProcess/M_PP_InteractOutline.M_PP_InteractOutline");
}

void JinzzaHighlight::SetHighlighted(AActor* Actor, bool bHighlighted)
{
	if (!Actor)
	{
		return;
	}

	// Meshes only - text labels / widget prompts / collision shapes shouldn't get an outline.
	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh->IsVisible())
		{
			Mesh->SetRenderCustomDepth(bHighlighted);
		}
	}
}

bool UjinzzaInteractHighlightSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && Super::ShouldCreateSubsystem(Outer);
}

void UjinzzaInteractHighlightSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UMaterialInterface* OutlineMaterial = LoadObject<UMaterialInterface>(nullptr, OutlineMaterialPath);
	if (!OutlineMaterial)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	OutlineVolume = InWorld.SpawnActor<APostProcessVolume>(Params);
	if (OutlineVolume)
	{
		// Blendables from every volume in effect stack, so this adds the outline on top of the
		// level's own post-process settings without overriding any of them.
		OutlineVolume->bUnbound = true;
		OutlineVolume->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, OutlineMaterial));
	}
}

UjinzzaFaceCameraComponent::UjinzzaFaceCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// After the camera has moved this frame, so labels don't lag a frame behind.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UjinzzaFaceCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const AActor* Owner = GetOwner();
	if (!PC || !PC->PlayerCameraManager || !Owner)
	{
		return;
	}
	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();

	auto FaceCamera = [&CameraLocation](USceneComponent* Component)
	{
		FVector ToCamera = CameraLocation - Component->GetComponentLocation();
		ToCamera.Z = 0.f;
		if (!ToCamera.IsNearlyZero())
		{
			Component->SetWorldRotation(FRotator(0.f, ToCamera.Rotation().Yaw, 0.f));
		}
	};

	TArray<UTextRenderComponent*> Texts;
	Owner->GetComponents(Texts);
	for (UTextRenderComponent* Text : Texts)
	{
		FaceCamera(Text);
	}

	TArray<UWidgetComponent*> Widgets;
	Owner->GetComponents(Widgets);
	for (UWidgetComponent* Widget : Widgets)
	{
		if (Widget->GetWidgetSpace() == EWidgetSpace::World)
		{
			FaceCamera(Widget);
		}
	}
}
