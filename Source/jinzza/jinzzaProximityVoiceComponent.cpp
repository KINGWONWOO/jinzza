// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaProximityVoiceComponent.h"
#include "jinzzaAudio.h"
#include "jinzzaCharacter.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaMegaphoneProp.h"
#include "jinzzaPartyPlayerState.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Sound/SoundEffectSource.h"
#include "SourceEffects/SourceEffectChorus.h"
#include "SourceEffects/SourceEffectFilter.h"
#include "SourceEffects/SourceEffectRingModulation.h"
#include "SourceEffects/SourceEffectSimpleDelay.h"

UjinzzaProximityVoiceComponent::UjinzzaProximityVoiceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UjinzzaProximityVoiceComponent::SetVoiceAnchor(USceneComponent* InAnchor)
{
	VoiceAnchor = InAnchor;
	Settings.ComponentToAttachTo = InAnchor;
}

void UjinzzaProximityVoiceComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickInterval(UpdateInterval);

	// Dedicated servers have no listener - nothing to play voice for.
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetComponentTickEnabled(false);
		return;
	}

	UpdateRegistration();
	UpdateVoiceSettings();
}

void UjinzzaProximityVoiceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateRegistration();
	UpdateVoiceSettings();
	UpdateActiveVoice();
}

AjinzzaCharacter* UjinzzaProximityVoiceComponent::GetCharacter() const
{
	return Cast<AjinzzaCharacter>(GetOwner());
}

void UjinzzaProximityVoiceComponent::UpdateRegistration()
{
	const AjinzzaCharacter* Character = GetCharacter();
	APlayerState* PlayerState = Character ? Character->GetPlayerState() : nullptr;
	if (!PlayerState || !PlayerState->GetUniqueId().IsValid())
	{
		return;
	}

	// UVOIPTalker's unregister removes the map entry by player id no matter who owns it, so an old pawn of the
	// same player being destroyed after this one registered (respawn, seamless travel) would silently steal
	// the slot back to nobody - re-check every update rather than registering once.
	if (RegisteredPlayerState.Get() != PlayerState || UVOIPStatics::GetVOIPTalkerForPlayer(PlayerState->GetUniqueId()) != this)
	{
		RegisterWithPlayerState(PlayerState);
		RegisteredPlayerState = PlayerState;
	}
}

EJinzzaVoiceFilter UjinzzaProximityVoiceComponent::GetEffectiveVoiceFilter() const
{
	const AjinzzaCharacter* Character = GetCharacter();
	if (!Character)
	{
		return EJinzzaVoiceFilter::None;
	}
	if (Character->IsStunned())
	{
		return EJinzzaVoiceFilter::Robot;
	}
	if (const AjinzzaPartyPlayerState* PartyState = Character->GetPlayerState<AjinzzaPartyPlayerState>())
	{
		return PartyState->GetVoiceFilter();
	}
	return EJinzzaVoiceFilter::None;
}

float UjinzzaProximityVoiceComponent::GetEffectiveVoiceRadius() const
{
	const AjinzzaCharacter* Character = GetCharacter();
	float Radius = VoiceRadius;
	if (Character && GetWorld())
	{
		// A handful of megaphones per level at most - cheaper than replicating a "held megaphone" pointer.
		for (TActorIterator<AjinzzaMegaphoneProp> It(GetWorld()); It; ++It)
		{
			if (It->IsHeldBy(Character) && It->IsAmplifying())
			{
				Radius = FMath::Max(Radius, It->GetAmplifiedVoiceRadius());
			}
		}
	}
	return Radius;
}

void UjinzzaProximityVoiceComponent::UpdateVoiceSettings()
{
	if (VoiceAnchor.IsValid())
	{
		Settings.ComponentToAttachTo = VoiceAnchor.Get();
	}

	const float Radius = GetEffectiveVoiceRadius();
	Settings.AttenuationSettings = JinzzaAudio::GetAttenuation(Radius);
	Settings.SourceEffectChain = GetFilterChain(GetEffectiveVoiceFilter());

	// Megaphone switched on/off mid-sentence: the radius can be changed on the playing voice directly.
	// (The filter chain can't - it's fixed per utterance, picked up on the next talk start.)
	UAudioComponent* Voice = ActiveVoiceComponent.Get();
	if (Voice && !FMath::IsNearlyEqual(Radius, ActiveVoiceRadius))
	{
		Voice->AdjustAttenuation(JinzzaAudio::MakeAttenuationSettings(Radius));
		ActiveVoiceRadius = Radius;
	}
}

void UjinzzaProximityVoiceComponent::UpdateActiveVoice()
{
	UAudioComponent* Voice = ActiveVoiceComponent.Get();
	if (!Voice)
	{
		return;
	}

	float Volume = 1.f;
	if (const UjinzzaGameUserSettings* UserSettings = UjinzzaGameUserSettings::Get())
	{
		// The engine's voice audio component isn't in the project's Voice sound class, so the Voice slider
		// is applied here directly.
		Volume *= UserSettings->GetVoiceVolume();
	}

	const AjinzzaCharacter* Character = GetCharacter();
	const AjinzzaPartyPlayerState* PartyState = Character ? Character->GetPlayerState<AjinzzaPartyPlayerState>() : nullptr;
	if (PartyState && PartyState->IsGhost())
	{
		Volume = 0.f;
	}

	const int32 Walls = Volume > 0.f ? CountWallsToListener() : 0;
	if (Walls > MaxWallsHeard)
	{
		Volume = 0.f;
	}
	else if (Walls > 0)
	{
		Volume *= FMath::Pow(VolumePerWall, static_cast<float>(Walls));
	}

	Voice->SetVolumeMultiplier(Volume);
	Voice->SetLowPassFilterEnabled(Walls > 0);
	Voice->SetLowPassFilterFrequency(WallLowPassFrequency);
}

int32 UjinzzaProximityVoiceComponent::CountWallsToListener() const
{
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const USceneComponent* Anchor = VoiceAnchor.Get();
	if (!PC || !PC->IsLocalController() || !Anchor)
	{
		return 0;
	}

	FVector ListenerLocation, FrontDir, RightDir;
	PC->GetAudioListenerPosition(ListenerLocation, FrontDir, RightDir);

	// Only static level geometry counts as a wall - players and thrown props don't block voice.
	const FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(JinzzaVoiceWalls), false);
	Params.AddIgnoredActor(GetOwner());
	Params.AddIgnoredActor(PC->GetPawn());

	// Walk from the listener to the speaker, hopping just past each wall hit.
	const FVector Target = Anchor->GetComponentLocation();
	FVector Start = ListenerLocation;
	int32 Walls = 0;
	FHitResult Hit;
	while (Walls <= MaxWallsHeard && World->LineTraceSingleByObjectType(Hit, Start, Target, ObjectParams, Params))
	{
		++Walls;
		// Ignore the rest of this actor (a wall mesh's far face) and continue from just past the hit.
		if (AActor* HitActor = Hit.GetActor())
		{
			Params.AddIgnoredActor(HitActor);
		}
		Start = Hit.ImpactPoint + (Target - Start).GetSafeNormal() * 5.f;
	}
	return Walls;
}

void UjinzzaProximityVoiceComponent::OnTalkingBegin(UAudioComponent* AudioComponent)
{
	ActiveVoiceComponent = AudioComponent;
	// The engine just created it from Settings, which UpdateVoiceSettings keeps current.
	ActiveVoiceRadius = GetEffectiveVoiceRadius();
	UpdateActiveVoice();

	Super::OnTalkingBegin(AudioComponent);
}

void UjinzzaProximityVoiceComponent::OnTalkingEnd()
{
	ActiveVoiceComponent.Reset();

	Super::OnTalkingEnd();
}

USoundEffectSourcePresetChain* UjinzzaProximityVoiceComponent::GetFilterChain(EJinzzaVoiceFilter Filter)
{
	if (Filter == EJinzzaVoiceFilter::None)
	{
		return nullptr;
	}

	if (TObjectPtr<USoundEffectSourcePresetChain>* Found = FilterChains.Find(Filter))
	{
		return *Found;
	}

	USoundEffectSourcePresetChain* Chain = NewObject<USoundEffectSourcePresetChain>(this);
	auto AddEffect = [Chain](USoundEffectSourcePreset* Preset)
	{
		FSourceEffectChainEntry Entry;
		Entry.Preset = Preset;
		Chain->Chain.Add(Entry);
	};

	switch (Filter)
	{
	case EJinzzaVoiceFilter::High:
	{
		// Thin, bright and slightly warbly - cuts the chest resonance a lower voice relies on.
		FSourceEffectFilterSettings FilterSettings;
		FilterSettings.FilterType = ESourceEffectFilterType::HighPass;
		FilterSettings.CutoffFrequency = 450.f;
		FilterSettings.FilterQ = 0.9f;
		USourceEffectFilterPreset* FilterPreset = NewObject<USourceEffectFilterPreset>(this);
		FilterPreset->SetSettings(FilterSettings);
		AddEffect(FilterPreset);

		FSourceEffectChorusBaseSettings ChorusSettings;
		ChorusSettings.Depth = 0.15f;
		ChorusSettings.Frequency = 6.f;
		ChorusSettings.Feedback = 0.1f;
		ChorusSettings.WetLevel = 0.35f;
		ChorusSettings.DryLevel = 0.8f;
		USourceEffectChorusPreset* ChorusPreset = NewObject<USourceEffectChorusPreset>(this);
		ChorusPreset->SetSettings(ChorusSettings);
		AddEffect(ChorusPreset);
		break;
	}
	case EJinzzaVoiceFilter::Low:
	{
		// Dark and thick - top end removed, slow deep chorus doubling.
		FSourceEffectFilterSettings FilterSettings;
		FilterSettings.FilterType = ESourceEffectFilterType::LowPass;
		FilterSettings.CutoffFrequency = 1200.f;
		FilterSettings.FilterQ = 1.f;
		USourceEffectFilterPreset* FilterPreset = NewObject<USourceEffectFilterPreset>(this);
		FilterPreset->SetSettings(FilterSettings);
		AddEffect(FilterPreset);

		FSourceEffectChorusBaseSettings ChorusSettings;
		ChorusSettings.Depth = 0.35f;
		ChorusSettings.Frequency = 0.6f;
		ChorusSettings.Feedback = 0.25f;
		ChorusSettings.WetLevel = 0.5f;
		ChorusSettings.DryLevel = 0.7f;
		USourceEffectChorusPreset* ChorusPreset = NewObject<USourceEffectChorusPreset>(this);
		ChorusPreset->SetSettings(ChorusSettings);
		AddEffect(ChorusPreset);
		break;
	}
	case EJinzzaVoiceFilter::Robot:
	{
		// Same values as the lobby voice test's "Robot" preset (UjinzzaVoiceTestWidget::OnRobotPresetClicked).
		constexpr float RobotAmount = 0.85f;
		constexpr float EchoAmount = 0.15f;

		FSourceEffectSimpleDelaySettings DelaySettings;
		DelaySettings.bDelayBasedOnDistance = false;
		DelaySettings.bUseDistanceOverride = false;
		DelaySettings.DelayAmount = 0.15f + 0.25f * EchoAmount;
		DelaySettings.DryAmount = 1.f;
		DelaySettings.WetAmount = EchoAmount;
		DelaySettings.Feedback = 0.3f * EchoAmount;
		USourceEffectSimpleDelayPreset* DelayPreset = NewObject<USourceEffectSimpleDelayPreset>(this);
		DelayPreset->SetSettings(DelaySettings);
		AddEffect(DelayPreset);

		FSourceEffectRingModulationSettings RingSettings;
		RingSettings.ModulatorType = ERingModulatorTypeSourceEffect::Sine;
		RingSettings.Frequency = 35.f;
		RingSettings.Depth = RobotAmount;
		RingSettings.DryLevel = 1.f - 0.8f * RobotAmount;
		RingSettings.WetLevel = RobotAmount;
		USourceEffectRingModulationPreset* RingPreset = NewObject<USourceEffectRingModulationPreset>(this);
		RingPreset->SetSettings(RingSettings);
		AddEffect(RingPreset);
		break;
	}
	default:
		break;
	}

	FilterChains.Add(Filter, Chain);
	return Chain;
}
