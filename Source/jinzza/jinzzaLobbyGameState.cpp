// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyGameState.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "jinzzaLobbyClock.h"

namespace
{
	struct FJinzzaTimeOfDayPreset
	{
		FRotator SunRotation;
		float SunIntensity = 0.f;
		FLinearColor SunColor = FLinearColor::White;
		float SkyIntensity = 0.f;
		FLinearColor SkyColor = FLinearColor::White;
	};

	// Day matches Lvl_Lobby's saved SunLight/SkyLight exactly (re-lit 2026-10-04): the sun comes in
	// from the north, through the curtained window wall, at 25 degrees, so a sunbeam lands on the
	// centre rug (the ready area). The level also has a SkyAtmosphere, a real-time-capture Movable
	// SkyLight, and warm point lights in the floor lamps / fireplace / bedroom that these presets
	// don't touch - at Night those carry the room. Sunset swings the sun lower and warmer through the
	// same windows; Night dims and cools the sun and sky. 2026-10-05 re-light: the Day sun went
	// 10 -> 15 and the sky fill is kept low (3.5) so the sun patch on the rug and the fireplace
	// stand out against a slightly dimmer room.
	const FJinzzaTimeOfDayPreset& GetPreset(EJinzzaLobbyTimeOfDay TimeOfDay)
	{
		static const FJinzzaTimeOfDayPreset Presets[] =
		{
			/* Day    */ { FRotator(-25.f, -100.f, 0.f), 15.f, FLinearColor::White,                      3.5f, FLinearColor::White },
			/* Sunset */ { FRotator(-10.f, -115.f, 0.f), 6.f, FLinearColor(1.0f, 0.55f, 0.25f),          1.8f, FLinearColor(1.0f, 0.65f, 0.45f) },
			/* Night  */ { FRotator(-40.f, -120.f, 0.f), 0.4f, FLinearColor(0.45f, 0.55f, 0.9f),         0.35f, FLinearColor(0.3f, 0.35f, 0.55f) },
		};
		return Presets[static_cast<uint8>(TimeOfDay)];
	}
}

void AjinzzaLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaLobbyGameState, MatchSettings);
	DOREPLIFETIME(AjinzzaLobbyGameState, TimeOfDay);
}

void AjinzzaLobbyGameState::BeginPlay()
{
	Super::BeginPlay();

	// See the header comment: without this, the room stays at whatever raw DirectionalLight_0/
	// SkyLight_0 values were last saved in the level - never guaranteed to actually match the
	// "Day" preset (or get the SkyLight's captured cubemap re-taken) until someone interacts with
	// the clock at least once.
	ApplyTimeOfDayVisuals();
}

void AjinzzaLobbyGameState::CycleTimeOfDay()
{
	if (!HasAuthority())
	{
		return;
	}

	const uint8 NextIndex = (static_cast<uint8>(TimeOfDay) + 1) % 3;
	TimeOfDay = static_cast<EJinzzaLobbyTimeOfDay>(NextIndex);
	ApplyTimeOfDayVisuals();
}

void AjinzzaLobbyGameState::OnRep_TimeOfDay()
{
	ApplyTimeOfDayVisuals();
}

void AjinzzaLobbyGameState::ApplyTimeOfDayVisuals()
{
	const FJinzzaTimeOfDayPreset& Preset = GetPreset(TimeOfDay);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		if (UDirectionalLightComponent* LightComponent = Cast<UDirectionalLightComponent>(It->GetLightComponent()))
		{
			It->SetActorRotation(Preset.SunRotation);
			LightComponent->SetIntensity(Preset.SunIntensity);
			LightComponent->SetLightColor(Preset.SunColor);
		}
		break;
	}

	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		if (USkyLightComponent* SkyLightComponent = It->GetLightComponent())
		{
			SkyLightComponent->SetIntensity(Preset.SkyIntensity);
			SkyLightComponent->SetLightColor(Preset.SkyColor);
			SkyLightComponent->RecaptureSky();
		}
		break;
	}

	for (TActorIterator<AjinzzaLobbyClock> It(World); It; ++It)
	{
		It->SetDisplayedTime(TimeOfDay);
		break;
	}
}
