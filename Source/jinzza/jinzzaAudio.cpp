// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaAudio.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/Package.h"

namespace
{
	constexpr float InnerRadiusFraction = 0.15f;
	constexpr float MinInnerRadius = 150.f;
	constexpr float VolumeAtEdgeDb = -40.f;
	constexpr float AirAbsorptionFrequencyNear = 20000.f;
	constexpr float AirAbsorptionFrequencyFar = 3000.f;

	// One cached attenuation asset per distinct radius (rounded to whole cm). Rooted so GC never collects them;
	// they're tiny and there are only a handful of distinct radii in the game.
	TMap<int32, USoundAttenuation*>& AttenuationCache()
	{
		static TMap<int32, USoundAttenuation*> Cache;
		return Cache;
	}
}

FSoundAttenuationSettings JinzzaAudio::MakeAttenuationSettings(float AudibleRadius)
{
	FSoundAttenuationSettings Settings;

	const float Radius = FMath::Max(AudibleRadius, 1.f);
	const float Inner = FMath::Min(FMath::Max(Radius * InnerRadiusFraction, MinInnerRadius), Radius * 0.5f);

	Settings.bAttenuate = true;
	Settings.bSpatialize = true;

	Settings.AttenuationShape = EAttenuationShape::Sphere;
	Settings.AttenuationShapeExtents = FVector(Inner, 0.f, 0.f);
	Settings.FalloffDistance = Radius - Inner;

	Settings.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	Settings.dBAttenuationAtMax = VolumeAtEdgeDb;
	Settings.FalloffMode = ENaturalSoundFalloffMode::Silent;

	Settings.bAttenuateWithLPF = true;
	Settings.LPFRadiusMin = Inner;
	Settings.LPFRadiusMax = Radius;
	Settings.LPFFrequencyAtMin = AirAbsorptionFrequencyNear;
	Settings.LPFFrequencyAtMax = AirAbsorptionFrequencyFar;

	return Settings;
}

USoundAttenuation* JinzzaAudio::GetAttenuation(float AudibleRadius)
{
	const int32 Key = FMath::RoundToInt(FMath::Max(AudibleRadius, 1.f));
	TMap<int32, USoundAttenuation*>& Cache = AttenuationCache();

	if (USoundAttenuation** Found = Cache.Find(Key))
	{
		if (*Found && (*Found)->IsValidLowLevel())
		{
			return *Found;
		}
	}

	USoundAttenuation* Attenuation = NewObject<USoundAttenuation>(GetTransientPackage(), NAME_None, RF_Transient);
	Attenuation->Attenuation = MakeAttenuationSettings(static_cast<float>(Key));
	Attenuation->AddToRoot();
	Cache.Add(Key, Attenuation);
	return Attenuation;
}

void JinzzaAudio::PlaySoundAt(const UObject* WorldContext, USoundBase* Sound, const FVector& Location, float AudibleRadius, float VolumeMultiplier)
{
	if (!Sound || !WorldContext)
	{
		return;
	}

	USoundAttenuation* Attenuation = IsAttenuated(AudibleRadius) ? GetAttenuation(AudibleRadius) : nullptr;
	UGameplayStatics::PlaySoundAtLocation(WorldContext, Sound, Location, FRotator::ZeroRotator, VolumeMultiplier, 1.f, 0.f, Attenuation);
}

bool JinzzaAudio::MakeOverrides(FSoundAttenuationSettings& Overrides, float AudibleRadius)
{
	if (!IsAttenuated(AudibleRadius))
	{
		return false;
	}

	Overrides = MakeAttenuationSettings(AudibleRadius);
	return true;
}

void JinzzaAudio::ApplyToComponent(UAudioComponent* Component, float AudibleRadius)
{
	if (Component && IsAttenuated(AudibleRadius))
	{
		Component->bAllowSpatialization = true;
		Component->bOverrideAttenuation = MakeOverrides(Component->AttenuationOverrides, AudibleRadius);
	}
}

float JinzzaAudio::CalcVolumeAtDistance(float AudibleRadius, float Distance)
{
	if (!IsAttenuated(AudibleRadius))
	{
		return 1.f;
	}

	const FSoundAttenuationSettings Settings = MakeAttenuationSettings(AudibleRadius);
	return Settings.Evaluate(FTransform::Identity, FVector(Distance, 0.f, 0.f));
}
