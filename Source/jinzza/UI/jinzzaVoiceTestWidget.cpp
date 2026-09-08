// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaVoiceTestWidget.h"
#include "jinzzaUIStyle.h"
#include "AudioCaptureComponent.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundEffectSource.h"
#include "SourceEffects/SourceEffectRingModulation.h"
#include "SourceEffects/SourceEffectSimpleDelay.h"

namespace
{
	// One row: a fill-width slider plus a small fixed-width value label to its right.
	USlider* AddSliderRow(UWidgetTree* Tree, UVerticalBox* Stack, const TCHAR* NamePrefix, const FText& RowLabel,
		float MinValue, float MaxValue, TObjectPtr<UTextBlock>& OutValueText)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(FString(NamePrefix) + TEXT("_Row")));

		USlider* Slider = Tree->ConstructWidget<USlider>(USlider::StaticClass(), *(FString(NamePrefix) + TEXT("_Slider")));
		Slider->SetMinValue(MinValue);
		Slider->SetMaxValue(MaxValue);
		if (UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(Slider))
		{
			SliderSlot->SetVerticalAlignment(VAlign_Center);
			SliderSlot->SetSize(ESlateSizeRule::Fill);
			SliderSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		}

		OutValueText = JinzzaUI::MakeBodyText(Tree, *(FString(NamePrefix) + TEXT("_Value")), FText::GetEmpty());
		if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutValueText))
		{
			ValueSlot->SetVerticalAlignment(VAlign_Center);
		}

		JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeLabeledRow(Tree, *(FString(NamePrefix) + TEXT("_LabeledRow")), RowLabel, Row));
		return Slider;
	}
}

void UjinzzaVoiceTestWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("Panel"));
	Panel->SetPadding(FMargin(24.f));
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* PanelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelBox"));
	PanelBox->SetWidthOverride(440.f);
	Panel->SetContent(PanelBox);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));
	PanelBox->AddChild(Stack);

	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeSectionHeading(WidgetTree, TEXT("Heading"), FText::FromString(TEXT("Voice Modulation Test"))), 0.f);
	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeDivider(WidgetTree, TEXT("HeaderDivider")));

	UTextBlock* Hint = JinzzaUI::MakeBodyText(WidgetTree, TEXT("Hint"), FText::FromString(TEXT("Press Speak & Listen, then talk into your mic.")), true);
	JinzzaUI::AddSpaced(Stack, Hint, 8.f);

	ToggleListenButton = JinzzaUI::MakePrimaryButton(WidgetTree, TEXT("ToggleListenButton"), FText::FromString(TEXT("Speak & Listen")));
	ToggleListenButtonText = nullptr;
	if (UTextBlock* InnerText = Cast<UTextBlock>(ToggleListenButton->GetChildAt(0)))
	{
		ToggleListenButtonText = InnerText;
	}
	JinzzaUI::AddSpaced(Stack, ToggleListenButton, 14.f);

	PitchSlider = AddSliderRow(WidgetTree, Stack, TEXT("Pitch"), FText::FromString(TEXT("Pitch")), 0.5f, 2.f, PitchValueText);
	RobotSlider = AddSliderRow(WidgetTree, Stack, TEXT("Robot"), FText::FromString(TEXT("Robot")), 0.f, 1.f, RobotValueText);
	EchoSlider = AddSliderRow(WidgetTree, Stack, TEXT("Echo"), FText::FromString(TEXT("Cave Echo")), 0.f, 1.f, EchoValueText);

	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeDivider(WidgetTree, TEXT("TemplateDivider")), 16.f);
	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeBodyText(WidgetTree, TEXT("TemplateLabel"), FText::FromString(TEXT("Templates")), true), 8.f);

	UHorizontalBox* TemplateRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TemplateRow"));
	JinzzaUI::AddSpaced(Stack, TemplateRow, 8.f);

	CaveButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("CaveButton"), FText::FromString(TEXT("Cave")), 16.f);
	if (UHorizontalBoxSlot* CaveSlot = TemplateRow->AddChildToHorizontalBox(CaveButton))
	{
		CaveSlot->SetSize(ESlateSizeRule::Fill);
		CaveSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	}

	HeliumButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("HeliumButton"), FText::FromString(TEXT("Helium")), 16.f);
	if (UHorizontalBoxSlot* HeliumSlot = TemplateRow->AddChildToHorizontalBox(HeliumButton))
	{
		HeliumSlot->SetSize(ESlateSizeRule::Fill);
		HeliumSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	}

	RobotPresetButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("RobotPresetButton"), FText::FromString(TEXT("Robot")), 16.f);
	if (UHorizontalBoxSlot* RobotSlot = TemplateRow->AddChildToHorizontalBox(RobotPresetButton))
	{
		RobotSlot->SetSize(ESlateSizeRule::Fill);
	}

	CloseButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("CloseButton"), FText::FromString(TEXT("Close")));
	if (UVerticalBoxSlot* CloseSlot = JinzzaUI::AddSpaced(Stack, CloseButton, 20.f))
	{
		CloseSlot->SetHorizontalAlignment(HAlign_Right);
	}
}

void UjinzzaVoiceTestWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

	if (ToggleListenButton)
	{
		ToggleListenButton->OnClicked.AddDynamic(this, &UjinzzaVoiceTestWidget::OnToggleListenClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UjinzzaVoiceTestWidget::OnCloseClicked);
	}
	if (PitchSlider)
	{
		PitchSlider->OnValueChanged.AddDynamic(this, &UjinzzaVoiceTestWidget::OnPitchChanged);
	}
	if (RobotSlider)
	{
		RobotSlider->OnValueChanged.AddDynamic(this, &UjinzzaVoiceTestWidget::OnRobotChanged);
	}
	if (EchoSlider)
	{
		EchoSlider->OnValueChanged.AddDynamic(this, &UjinzzaVoiceTestWidget::OnEchoChanged);
	}
	if (CaveButton)
	{
		CaveButton->OnClicked.AddDynamic(this, &UjinzzaVoiceTestWidget::OnCaveClicked);
	}
	if (HeliumButton)
	{
		HeliumButton->OnClicked.AddDynamic(this, &UjinzzaVoiceTestWidget::OnHeliumClicked);
	}
	if (RobotPresetButton)
	{
		RobotPresetButton->OnClicked.AddDynamic(this, &UjinzzaVoiceTestWidget::OnRobotPresetClicked);
	}

	ApplyPreset(1.f, 0.f, 0.f);
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

void UjinzzaVoiceTestWidget::OnCloseClicked()
{
	RemoveFromParent();
}

void UjinzzaVoiceTestWidget::EnsureEffectChain()
{
	if (EffectChain)
	{
		return;
	}

	EffectChain = NewObject<USoundEffectSourcePresetChain>(this);

	RingModPreset = NewObject<USourceEffectRingModulationPreset>(this);
	DelayPreset = NewObject<USourceEffectSimpleDelayPreset>(this);

	FSourceEffectChainEntry RingEntry;
	RingEntry.Preset = RingModPreset;
	RingEntry.bBypass = false;

	FSourceEffectChainEntry DelayEntry;
	DelayEntry.Preset = DelayPreset;
	DelayEntry.bBypass = false;

	EffectChain->Chain = { DelayEntry, RingEntry };

	ApplyRobotSettings();
	ApplyEchoSettings();
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

	EnsureEffectChain();

	if (!CaptureComponent)
	{
		CaptureComponent = NewObject<UAudioCaptureComponent>(PC);
		CaptureComponent->RegisterComponentWithWorld(GetWorld());
	}

	CaptureComponent->SourceEffectChain = EffectChain;
	CaptureComponent->CreateAudioComponent();
	ApplyPitch();
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

void UjinzzaVoiceTestWidget::OnPitchChanged(float NewValue)
{
	CurrentPitch = NewValue;
	RefreshValueLabels();
	ApplyPitch();
}

void UjinzzaVoiceTestWidget::OnRobotChanged(float NewValue)
{
	CurrentRobotAmount = NewValue;
	RefreshValueLabels();
	ApplyRobotSettings();
}

void UjinzzaVoiceTestWidget::OnEchoChanged(float NewValue)
{
	CurrentEchoAmount = NewValue;
	RefreshValueLabels();
	ApplyEchoSettings();
}

void UjinzzaVoiceTestWidget::ApplyPreset(float NewPitch, float NewRobot, float NewEcho)
{
	CurrentPitch = NewPitch;
	CurrentRobotAmount = NewRobot;
	CurrentEchoAmount = NewEcho;

	if (PitchSlider) PitchSlider->SetValue(CurrentPitch);
	if (RobotSlider) RobotSlider->SetValue(CurrentRobotAmount);
	if (EchoSlider) EchoSlider->SetValue(CurrentEchoAmount);

	RefreshValueLabels();
	ApplyPitch();
	ApplyRobotSettings();
	ApplyEchoSettings();
}

void UjinzzaVoiceTestWidget::OnCaveClicked()
{
	// Deep, slowed-down pitch plus a long, feeding-back echo for a cavernous tail.
	ApplyPreset(0.65f, 0.f, 0.6f);
}

void UjinzzaVoiceTestWidget::OnHeliumClicked()
{
	// Classic chipmunk/helium effect - pitch alone, no other DSP.
	ApplyPreset(1.7f, 0.f, 0.f);
}

void UjinzzaVoiceTestWidget::OnRobotPresetClicked()
{
	// Slightly flattened pitch plus a strong ring-modulator buzz, a touch of echo for texture.
	ApplyPreset(0.9f, 0.85f, 0.15f);
}

void UjinzzaVoiceTestWidget::ApplyPitch()
{
	if (CaptureComponent)
	{
		if (UAudioComponent* AudioComp = CaptureComponent->GetAudioComponent())
		{
			AudioComp->SetPitchMultiplier(CurrentPitch);
		}

		// PitchMultiplier is read when the synth (re)starts generating - restart to pick up a
		// change made while already listening (Robot/Echo don't need this - see ApplyRobotSettings/
		// ApplyEchoSettings, which push live updates through the preset objects instead).
		if (bIsListening)
		{
			CaptureComponent->Stop();
			CaptureComponent->Start();
		}
	}
}

void UjinzzaVoiceTestWidget::ApplyRobotSettings()
{
	if (!RingModPreset)
	{
		return;
	}

	FSourceEffectRingModulationSettings Settings;
	Settings.ModulatorType = ERingModulatorTypeSourceEffect::Sine;
	Settings.Frequency = 35.f;
	Settings.Depth = CurrentRobotAmount;
	Settings.DryLevel = 1.f - 0.8f * CurrentRobotAmount;
	Settings.WetLevel = CurrentRobotAmount;
	RingModPreset->SetSettings(Settings);
}

void UjinzzaVoiceTestWidget::ApplyEchoSettings()
{
	if (!DelayPreset)
	{
		return;
	}

	FSourceEffectSimpleDelaySettings Settings;
	Settings.bDelayBasedOnDistance = false;
	Settings.bUseDistanceOverride = false;
	Settings.DelayAmount = 0.15f + 0.25f * CurrentEchoAmount;
	Settings.DryAmount = 1.f;
	Settings.WetAmount = CurrentEchoAmount;
	Settings.Feedback = 0.3f * CurrentEchoAmount;
	DelayPreset->SetSettings(Settings);
}

void UjinzzaVoiceTestWidget::RefreshValueLabels()
{
	if (PitchValueText)
	{
		PitchValueText->SetText(FText::FromString(FString::Printf(TEXT("x%.2f"), CurrentPitch)));
	}
	if (RobotValueText)
	{
		RobotValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(CurrentRobotAmount * 100.f))));
	}
	if (EchoValueText)
	{
		EchoValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(CurrentEchoAmount * 100.f))));
	}
}
