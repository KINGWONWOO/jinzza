// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaVoiceTestWidget.h"
#include "AudioCaptureComponent.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

namespace
{
	constexpr float HighPitchMultiplier = 1.5f;
	constexpr float LowPitchMultiplier = 0.7f;
}

void UjinzzaVoiceTestWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ToggleListenButton)
	{
		ToggleListenButton->OnClicked.AddDynamic(this, &UjinzzaVoiceTestWidget::OnToggleListenClicked);
	}
	if (NextFilterButton)
	{
		NextFilterButton->OnClicked.AddDynamic(this, &UjinzzaVoiceTestWidget::OnNextFilterClicked);
	}
	if (PrevFilterButton)
	{
		PrevFilterButton->OnClicked.AddDynamic(this, &UjinzzaVoiceTestWidget::OnPrevFilterClicked);
	}

	RefreshFilterText();
}

void UjinzzaVoiceTestWidget::NativeDestruct()
{
	StopListening();

	if (CaptureComponent)
	{
		CaptureComponent->DestroyComponent();
		CaptureComponent = nullptr;
	}

	Super::NativeDestruct();
}

void UjinzzaVoiceTestWidget::OnToggleListenClicked()
{
	if (bIsListening)
	{
		StopListening();
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	if (!CaptureComponent)
	{
		CaptureComponent = NewObject<UAudioCaptureComponent>(PC);
		CaptureComponent->RegisterComponentWithWorld(GetWorld());
	}

	CaptureComponent->CreateAudioComponent();
	ApplyCurrentFilter();
	CaptureComponent->Start();
	bIsListening = true;

	if (ToggleListenButtonText)
	{
		ToggleListenButtonText->SetText(FText::FromString(TEXT("Stop Listening")));
	}
}

void UjinzzaVoiceTestWidget::StopListening()
{
	if (CaptureComponent)
	{
		CaptureComponent->Stop();
	}
	bIsListening = false;

	if (ToggleListenButtonText)
	{
		ToggleListenButtonText->SetText(FText::FromString(TEXT("Speak & Listen")));
	}
}

void UjinzzaVoiceTestWidget::OnNextFilterClicked()
{
	CurrentFilter = static_cast<EJinzzaVoiceFilter>((static_cast<uint8>(CurrentFilter) + 1) % 4);
	RefreshFilterText();
	if (bIsListening)
	{
		ApplyCurrentFilter();
	}
}

void UjinzzaVoiceTestWidget::OnPrevFilterClicked()
{
	CurrentFilter = static_cast<EJinzzaVoiceFilter>((static_cast<uint8>(CurrentFilter) + 3) % 4);
	RefreshFilterText();
	if (bIsListening)
	{
		ApplyCurrentFilter();
	}
}

void UjinzzaVoiceTestWidget::RefreshFilterText()
{
	if (FilterText)
	{
		FilterText->SetText(GetFilterDisplayName(CurrentFilter));
	}
}

FText UjinzzaVoiceTestWidget::GetFilterDisplayName(EJinzzaVoiceFilter Filter)
{
	switch (Filter)
	{
	case EJinzzaVoiceFilter::High:  return FText::FromString(TEXT("High"));
	case EJinzzaVoiceFilter::Low:   return FText::FromString(TEXT("Low"));
	case EJinzzaVoiceFilter::Robot: return FText::FromString(TEXT("Robot"));
	default:                        return FText::FromString(TEXT("None"));
	}
}

void UjinzzaVoiceTestWidget::ApplyCurrentFilter()
{
	if (!CaptureComponent)
	{
		return;
	}

	UAudioComponent* AudioComp = CaptureComponent->GetAudioComponent();
	if (AudioComp)
	{
		AudioComp->SetPitchMultiplier(1.f);
	}

	switch (CurrentFilter)
	{
	case EJinzzaVoiceFilter::High:
		if (AudioComp)
		{
			AudioComp->SetPitchMultiplier(HighPitchMultiplier);
		}
		break;

	case EJinzzaVoiceFilter::Low:
		if (AudioComp)
		{
			AudioComp->SetPitchMultiplier(LowPitchMultiplier);
		}
		break;

	case EJinzzaVoiceFilter::Robot:
		// Real ring-modulation needs USourceEffectRingModulationPreset (Synthesis plugin module),
		// which isn't a jinzza.Build.cs dependency yet - adding one forces a full UnrealBuildTool
		// rebuild (not Live Coding), which needs the editor fully closed. Until that's done at a
		// convenient time, Robot plays back at neutral pitch same as None (a labeled, harmless
		// placeholder - same "wire content later" pattern as ScoreSound/montages elsewhere).
	case EJinzzaVoiceFilter::None:
	default:
		break;
	}

	// PitchMultiplier is read when the synth (re)starts generating - restart to pick up a change
	// made while already listening.
	if (bIsListening)
	{
		CaptureComponent->Stop();
		CaptureComponent->Start();
	}
}
