// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaVoiceTestWidget.h"
#include "jinzzaUIStyle.h"
#include "jinzzaMicLoopbackComponent.h"
#include "jinzzaGameUserSettings.h"
#include "jinzza.h"
#include "AudioCaptureCore.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Components/ComboBoxString.h"
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
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundEffectSource.h"
#include "SourceEffects/SourceEffectRingModulation.h"
#include "SourceEffects/SourceEffectSimpleDelay.h"

namespace
{
	/** Width of the mic level meter track (matches the row control width below). */
	constexpr float VoiceTestMeterWidth = 400.f;

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
		JinzzaUI::ApplyStickerStyle(Slider);

		// Big yellow value readout, like the Settings sliders.
		OutValueText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(FString(NamePrefix) + TEXT("_Value")));
		OutValueText->SetFont(JinzzaUI::HeadingFont(22));
		OutValueText->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_Yellow));
		OutValueText->SetJustification(ETextJustify::Right);
		USizeBox* ValueBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(FString(NamePrefix) + TEXT("_ValueBox")));
		ValueBox->SetWidthOverride(80.f);
		ValueBox->AddChild(OutValueText);
		if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueBox))
		{
			ValueSlot->SetVerticalAlignment(VAlign_Center);
		}

		JinzzaUI::AddStickerRow(Tree, Stack, *(FString(NamePrefix) + TEXT("_LabeledRow")), RowLabel, Row, 400.f);
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

	// Sticker-style panel, matching the main menu / T_Logo.
	UBorder* PanelFace = nullptr;
	UOverlay* Panel = JinzzaUI::MakeStickerPanel(WidgetTree, TEXT("Panel"), PanelFace);
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* PanelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelBox"));
	PanelBox->SetWidthOverride(720.f);
	PanelFace->SetContent(PanelBox);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));
	PanelBox->AddChild(Stack);

	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeStickerHeading(WidgetTree, TEXT("Heading"), FText::FromString(TEXT("Voice Test")), 48), 0.f);

	UTextBlock* Hint = JinzzaUI::MakeStickerText(WidgetTree, TEXT("Hint"), FText::FromString(TEXT("Press Speak & Listen, then talk into your mic.")), 20, true);
	JinzzaUI::AddSpaced(Stack, Hint, 8.f);

	// --- Devices: which mic to record from, which speakers/headphones to play through, and how
	// much to boost the mic so you can actually hear yourself. ---
	JinzzaUI::AddStickerSection(WidgetTree, Stack, TEXT("DevicesSection"), FText::FromString(TEXT("Devices")));

	MicDeviceCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MicDeviceCombo"));
	JinzzaUI::ApplyStickerStyle(MicDeviceCombo);
	JinzzaUI::AddStickerRow(WidgetTree, Stack, TEXT("MicDeviceRow"), FText::FromString(TEXT("Mic")), MicDeviceCombo, 400.f);

	OutputDeviceCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("OutputDeviceCombo"));
	JinzzaUI::ApplyStickerStyle(OutputDeviceCombo);
	JinzzaUI::AddStickerRow(WidgetTree, Stack, TEXT("OutputDeviceRow"), FText::FromString(TEXT("Speaker")), OutputDeviceCombo, 400.f);

	BoostSlider = AddSliderRow(WidgetTree, Stack, TEXT("Boost"), FText::FromString(TEXT("Mic Boost")), 1.f, 6.f, BoostValueText);

	ToggleListenButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("ToggleListenButton"), FText::FromString(TEXT("Speak & Listen")), JinzzaUI::Sticker_Teal, 28.f, true);
	ToggleListenButtonText = nullptr;
	if (UTextBlock* InnerText = Cast<UTextBlock>(ToggleListenButton->GetChildAt(0)))
	{
		ToggleListenButtonText = InnerText;
	}
	JinzzaUI::AddSpaced(Stack, ToggleListenButton, 18.f);

	// Live mic level meter: a dim track with a yellow fill whose width follows the mic (NativeTick).
	USizeBox* MeterTrackBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MeterTrackBox"));
	MeterTrackBox->SetWidthOverride(VoiceTestMeterWidth);
	MeterTrackBox->SetHeightOverride(22.f);
	UOverlay* MeterOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MeterOverlay"));
	MeterTrackBox->AddChild(MeterOverlay);
	UBorder* MeterTrack = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MeterTrack"));
	MeterTrack->SetBrush(FSlateRoundedBoxBrush(JinzzaUI::Sticker_White.CopyWithNewOpacity(0.2f), 11.f));
	if (UOverlaySlot* TrackSlot = MeterOverlay->AddChildToOverlay(MeterTrack))
	{
		TrackSlot->SetHorizontalAlignment(HAlign_Fill);
		TrackSlot->SetVerticalAlignment(VAlign_Fill);
	}
	LevelFill = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MeterFillBox"));
	LevelFill->SetWidthOverride(0.f);
	UBorder* MeterFill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MeterFill"));
	MeterFill->SetBrush(FSlateRoundedBoxBrush(JinzzaUI::Sticker_Yellow, 11.f));
	LevelFill->AddChild(MeterFill);
	if (UOverlaySlot* FillSlot = MeterOverlay->AddChildToOverlay(LevelFill))
	{
		FillSlot->SetHorizontalAlignment(HAlign_Left);
		FillSlot->SetVerticalAlignment(VAlign_Fill);
	}
	JinzzaUI::AddStickerRow(WidgetTree, Stack, TEXT("MicLevelRow"), FText::FromString(TEXT("Mic Level")), MeterTrackBox, 400.f);

	JinzzaUI::AddStickerSection(WidgetTree, Stack, TEXT("EffectsSection"), FText::FromString(TEXT("Effects")));
	PitchSlider = AddSliderRow(WidgetTree, Stack, TEXT("Pitch"), FText::FromString(TEXT("Pitch")), 0.5f, 2.f, PitchValueText);
	RobotSlider = AddSliderRow(WidgetTree, Stack, TEXT("Robot"), FText::FromString(TEXT("Robot")), 0.f, 1.f, RobotValueText);
	EchoSlider = AddSliderRow(WidgetTree, Stack, TEXT("Echo"), FText::FromString(TEXT("Cave Echo")), 0.f, 1.f, EchoValueText);

	JinzzaUI::AddStickerSection(WidgetTree, Stack, TEXT("PresetSection"), FText::FromString(TEXT("Presets")));

	UHorizontalBox* TemplateRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TemplateRow"));
	JinzzaUI::AddSpaced(Stack, TemplateRow, 10.f);

	CaveButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("CaveButton"), FText::FromString(TEXT("Cave")), JinzzaUI::Sticker_Teal, 22.f);
	if (UHorizontalBoxSlot* CaveSlot = TemplateRow->AddChildToHorizontalBox(CaveButton))
	{
		CaveSlot->SetSize(ESlateSizeRule::Fill);
		CaveSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	}

	HeliumButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("HeliumButton"), FText::FromString(TEXT("Helium")), JinzzaUI::Sticker_Teal, 22.f);
	if (UHorizontalBoxSlot* HeliumSlot = TemplateRow->AddChildToHorizontalBox(HeliumButton))
	{
		HeliumSlot->SetSize(ESlateSizeRule::Fill);
		HeliumSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	}

	RobotPresetButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("RobotPresetButton"), FText::FromString(TEXT("Robot")), JinzzaUI::Sticker_Teal, 22.f);
	if (UHorizontalBoxSlot* RobotSlot = TemplateRow->AddChildToHorizontalBox(RobotPresetButton))
	{
		RobotSlot->SetSize(ESlateSizeRule::Fill);
	}

	CloseButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("CloseButton"), FText::FromString(TEXT("Back")), JinzzaUI::Sticker_Sky, 26.f);
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

	if (BoostSlider)
	{
		BoostSlider->SetValue(CurrentBoost);
		BoostSlider->OnValueChanged.AddDynamic(this, &UjinzzaVoiceTestWidget::OnBoostChanged);
	}

	PopulateMicDevices();
	if (MicDeviceCombo)
	{
		MicDeviceCombo->OnSelectionChanged.AddDynamic(this, &UjinzzaVoiceTestWidget::OnMicDeviceSelected);
	}

	// Output devices come back asynchronously - see OnOutputDevicesObtained.
	if (OutputDeviceCombo)
	{
		OutputDeviceCombo->AddOption(TEXT("Loading..."));
		OutputDeviceCombo->SetSelectedIndex(0);
		OutputDeviceCombo->OnSelectionChanged.AddDynamic(this, &UjinzzaVoiceTestWidget::OnOutputDeviceSelected);
		FOnAudioOutputDevicesObtained Obtained;
		Obtained.BindDynamic(this, &UjinzzaVoiceTestWidget::OnOutputDevicesObtained);
		UAudioMixerBlueprintLibrary::GetAvailableAudioOutputDevices(this, Obtained);
	}

	ApplyPreset(1.f, 0.f, 0.f);
}

void UjinzzaVoiceTestWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Meter on a dB scale (-50 dB .. 0 dB -> empty .. full) so normal speech visibly moves it;
	// fast attack, slower fall so it reads smoothly.
	float Target = 0.f;
	if (bIsListening && CaptureComponent)
	{
		const float Level = CaptureComponent->GetInputLevel();
		if (Level > 0.f)
		{
			Target = FMath::Clamp((20.f * FMath::LogX(10.f, Level) + 50.f) / 50.f, 0.f, 1.f);
		}
	}
	DisplayLevel = Target > DisplayLevel ? Target : FMath::Max(Target, DisplayLevel - InDeltaTime * 1.5f);

	if (LevelFill)
	{
		LevelFill->SetWidthOverride(DisplayLevel * VoiceTestMeterWidth);
	}
}

void UjinzzaVoiceTestWidget::PopulateMicDevices()
{
	if (!MicDeviceCombo)
	{
		return;
	}

	MicDeviceCombo->ClearOptions();
	MicDeviceIndices = { INDEX_NONE };
	MicDeviceIds = { FString() };
	MicDeviceCombo->AddOption(TEXT("System Default"));

	Audio::FAudioCapture Capture;
	TArray<Audio::FCaptureDeviceInfo> Devices;
	Capture.GetCaptureDevicesAvailable(Devices);
	for (int32 Index = 0; Index < Devices.Num(); ++Index)
	{
		MicDeviceCombo->AddOption(Devices[Index].DeviceName);
		MicDeviceIndices.Add(Index);
		MicDeviceIds.Add(Devices[Index].DeviceId);
	}

	// Start on the mic saved in settings (shared with the Settings screen's Mic Device).
	const UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	const FString SavedId = Settings ? Settings->GetMicDeviceId() : FString();
	const int32 SavedOption = SavedId.IsEmpty() ? 0 : FMath::Max(0, MicDeviceIds.IndexOfByKey(SavedId));
	SelectedMicDeviceIndex = MicDeviceIndices[SavedOption];
	MicDeviceCombo->SetSelectedIndex(SavedOption);
}

void UjinzzaVoiceTestWidget::OnMicDeviceSelected(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	// Direct = set from code (PopulateMicDevices), not a user pick.
	if (SelectionType == ESelectInfo::Direct || !MicDeviceCombo)
	{
		return;
	}

	const int32 Option = MicDeviceCombo->GetSelectedIndex();
	if (!MicDeviceIndices.IsValidIndex(Option))
	{
		return;
	}

	SelectedMicDeviceIndex = MicDeviceIndices[Option];
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetMicDeviceId(MicDeviceIds[Option]);
		Settings->SaveSettings();
	}

	RecreateCapture();
}

void UjinzzaVoiceTestWidget::OnOutputDevicesObtained(const TArray<FAudioOutputDeviceInfo>& AvailableDevices)
{
	if (!OutputDeviceCombo)
	{
		return;
	}

	OutputDeviceCombo->ClearOptions();
	OutputDeviceIds.Reset();
	int32 CurrentOption = 0;
	for (const FAudioOutputDeviceInfo& Device : AvailableDevices)
	{
		if (Device.bIsCurrentDevice)
		{
			CurrentOption = OutputDeviceIds.Num();
		}
		OutputDeviceCombo->AddOption(Device.bIsSystemDefault ? FString::Printf(TEXT("%s (Default)"), *Device.Name) : Device.Name);
		OutputDeviceIds.Add(Device.DeviceId);
	}

	if (OutputDeviceIds.Num() == 0)
	{
		OutputDeviceCombo->AddOption(TEXT("System Default"));
		OutputDeviceIds.Add(FString());
	}
	OutputDeviceCombo->SetSelectedIndex(CurrentOption);
}

void UjinzzaVoiceTestWidget::OnOutputDeviceSelected(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct || !OutputDeviceCombo)
	{
		return;
	}

	const int32 Option = OutputDeviceCombo->GetSelectedIndex();
	if (!OutputDeviceIds.IsValidIndex(Option))
	{
		return;
	}

	// Moves ALL game audio to the chosen device right away (the loopback included).
	FOnCompletedDeviceSwap Swapped;
	Swapped.BindDynamic(this, &UjinzzaVoiceTestWidget::OnOutputDeviceSwapped);
	UAudioMixerBlueprintLibrary::SwapAudioOutputDevice(this, OutputDeviceIds[Option], Swapped);
}

void UjinzzaVoiceTestWidget::OnOutputDeviceSwapped(const FSwapAudioOutputResult& SwapResult)
{
	UE_LOG(Logjinzza, Log, TEXT("VoiceTest: output device swap to '%s' finished (result %d)."),
		*SwapResult.RequestedDeviceId, static_cast<int32>(SwapResult.Result));
}

void UjinzzaVoiceTestWidget::OnBoostChanged(float NewValue)
{
	CurrentBoost = NewValue;
	RefreshValueLabels();
	if (CaptureComponent)
	{
		CaptureComponent->SetGain(CurrentBoost);
	}
}

void UjinzzaVoiceTestWidget::RecreateCapture()
{
	const bool bWasListening = bIsListening;
	StopListening();
	if (CaptureComponent)
	{
		CaptureComponent->DestroyComponent();
		CaptureComponent = nullptr;
	}
	if (bWasListening)
	{
		StartListening();
	}
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
	// Unlike the kiosk popup (destroyed on close), the main menu keeps this widget alive as a
	// switcher page across Back navigation - stop the mic capture explicitly so it doesn't keep
	// running silently in the background after leaving the page.
	StopListening();
	OnBackRequested.Broadcast();
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

	StartListening();
}

void UjinzzaVoiceTestWidget::StartListening()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	EnsureEffectChain();

	if (!CaptureComponent)
	{
		CaptureComponent = NewObject<UjinzzaMicLoopbackComponent>(PC);
		CaptureComponent->DeviceIndex = SelectedMicDeviceIndex;
		CaptureComponent->RegisterComponentWithWorld(GetWorld());
	}
	CaptureComponent->SetGain(CurrentBoost);

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
	// A bit deeper, plus a roomy echo - toned down so the voice stays clear.
	ApplyPreset(0.85f, 0.f, 0.55f);
}

void UjinzzaVoiceTestWidget::OnHeliumClicked()
{
	// Chipmunk/helium - pitch alone, no other DSP.
	ApplyPreset(1.45f, 0.f, 0.f);
}

void UjinzzaVoiceTestWidget::OnRobotPresetClicked()
{
	// Slightly flattened pitch plus a ring-modulator buzz (voice still on top), a touch of echo.
	ApplyPreset(0.95f, 0.75f, 0.1f);
}

void UjinzzaVoiceTestWidget::ApplyPitch()
{
	// Live: the loopback's own pitch shifter keeps playback real-time, so there's no restart (the
	// old Stop/Start here is what cut the voice out while dragging a slider mid-test).
	if (CaptureComponent)
	{
		CaptureComponent->SetPitch(CurrentPitch);
	}
}

void UjinzzaVoiceTestWidget::ApplyRobotSettings()
{
	if (!RingModPreset)
	{
		return;
	}

	// Gentler than before (which went almost fully wet and buried the voice): even at 100% the dry
	// voice stays at half level under the buzz, so words stay understandable.
	FSourceEffectRingModulationSettings Settings;
	Settings.ModulatorType = ERingModulatorTypeSourceEffect::Sine;
	Settings.Frequency = 40.f;
	Settings.Depth = 1.f;
	Settings.DryLevel = 1.f - 0.5f * CurrentRobotAmount;
	Settings.WetLevel = 0.7f * CurrentRobotAmount;
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
	// Shorter, quieter echo than before so it reads as a room/cave around the voice instead of
	// a second voice talking over it.
	Settings.DelayAmount = 0.12f + 0.18f * CurrentEchoAmount;
	Settings.DryAmount = 1.f;
	Settings.WetAmount = 0.55f * CurrentEchoAmount;
	Settings.Feedback = 0.35f * CurrentEchoAmount;
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
	if (BoostValueText)
	{
		BoostValueText->SetText(FText::FromString(FString::Printf(TEXT("%.1fx"), CurrentBoost)));
	}
}
