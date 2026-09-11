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

	// Day matches this room's existing hand-tuned sun angle exactly (see [[lobby-noobgame-decor]] -
	// the windows were arranged around this exact rotation), Sunset/Night are new presets that swing
	// the sun down toward the horizon and cool the ambient sky for evening/night.
	const FJinzzaTimeOfDayPreset& GetPreset(EJinzzaLobbyTimeOfDay TimeOfDay)
	{
		static const FJinzzaTimeOfDayPreset Presets[] =
		{
			/* Day    */ { FRotator(-50.f, 20.f, 0.f), 10.f, FLinearColor::White,                        1.0f, FLinearColor::White },
			/* Sunset */ { FRotator(-8.f, 20.f, 0.f),   6.f, FLinearColor(1.0f, 0.55f, 0.25f),            0.6f, FLinearColor(1.0f, 0.65f, 0.45f) },
			/* Night  */ { FRotator(-70.f, 20.f, 0.f),  0.4f, FLinearColor(0.45f, 0.55f, 0.9f),           0.12f, FLinearColor(0.3f, 0.35f, 0.55f) },
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
