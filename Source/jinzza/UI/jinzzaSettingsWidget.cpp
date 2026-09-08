// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaSettingsWidget.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaUIStyle.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Widget.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"
#include "AudioCaptureBlueprintLibrary.h"

namespace
{
	enum ETabPage : int32
	{
		Tab_Graphics = 0,
		Tab_Audio = 1,
		Tab_Controls = 2,
		Tab_Gameplay = 3,
	};

	FString ResolutionToString(const FIntPoint& Res)
	{
		return FString::Printf(TEXT("%d x %d"), Res.X, Res.Y);
	}

	struct FTabRowWidgets
	{
		UButton* Button = nullptr;
		UWidget* Accent = nullptr;
		UWidget* Row = nullptr;
	};

	/** Thin sidebar row: a 4px accent bar (toggled by SetActiveTab) + a fill-width tab button. */
	FTabRowWidgets MakeTabRow(UWidgetTree* Tree, FName Name, const FText& Label)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Name.ToString() + TEXT("Row")));

		USizeBox* AccentBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("AccentBox")));
		AccentBox->SetWidthOverride(4.f);
		UBorder* Accent = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("Accent")));
		Accent->SetBrush(FSlateColorBrush(JinzzaUI::Color_Accent));
		AccentBox->AddChild(Accent);

		if (UHorizontalBoxSlot* AccentSlot = Row->AddChildToHorizontalBox(AccentBox))
		{
			AccentSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		}

		UButton* Button = JinzzaUI::MakeSecondaryButton(Tree, *(Name.ToString() + TEXT("Button")), Label, 16.f);
		if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetSize(ESlateSizeRule::Fill);
		}

		FTabRowWidgets Result;
		Result.Button = Button;
		Result.Accent = Accent;
		Result.Row = Row;
		return Result;
	}

	/** Side-by-side rebind button + current-key label, wrapped so it can sit as one control in a labeled row. */
	UWidget* MakeRebindControl(UWidgetTree* Tree, UButton* Button, UTextBlock* Label)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Button->GetFName().ToString() + TEXT("_Wrap")));
		if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label))
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Row;
	}
}

void UjinzzaSettingsWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrush(FSlateColorBrush(JinzzaUI::Color_Background));
	Background->SetHorizontalAlignment(HAlign_Fill);
	Background->SetVerticalAlignment(VAlign_Fill);
	WidgetTree->RootWidget = Background;

	UHorizontalBox* MainRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainRow"));
	Background->SetContent(MainRow);

	// --- Sidebar ---
	USizeBox* SidebarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SidebarBox"));
	SidebarBox->SetWidthOverride(160.f);
	MainRow->AddChildToHorizontalBox(SidebarBox);

	UVerticalBox* Sidebar = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Sidebar"));
	SidebarBox->AddChild(Sidebar);

	auto AddTabRow = [&](FName Name, const FText& Label, TObjectPtr<UButton>& OutButton, TObjectPtr<UWidget>& OutAccent)
	{
		const FTabRowWidgets Widgets = MakeTabRow(WidgetTree, Name, Label);
		OutButton = Widgets.Button;
		OutAccent = Widgets.Accent;
		if (UVerticalBoxSlot* Slot = Sidebar->AddChildToVerticalBox(Widgets.Row))
		{
			Slot->SetPadding(FMargin(0.f, 4.f));
		}
	};

	AddTabRow(TEXT("Graphics"), FText::FromString(TEXT("Graphics")), GraphicsTabButton, GraphicsTabAccent);
	AddTabRow(TEXT("Audio"), FText::FromString(TEXT("Audio")), AudioTabButton, AudioTabAccent);
	AddTabRow(TEXT("Controls"), FText::FromString(TEXT("Controls")), ControlsTabButton, ControlsTabAccent);
	AddTabRow(TEXT("Gameplay"), FText::FromString(TEXT("Gameplay")), GameplayTabButton, GameplayTabAccent);

	// --- Content column ---
	UVerticalBox* ContentColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentColumn"));
	if (UHorizontalBoxSlot* ContentSlot = MainRow->AddChildToHorizontalBox(ContentColumn))
	{
		ContentSlot->SetSize(ESlateSizeRule::Fill);
		ContentSlot->SetPadding(FMargin(20.f));
	}

	TabSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("TabSwitcher"));
	if (UVerticalBoxSlot* SwitcherSlot = ContentColumn->AddChildToVerticalBox(TabSwitcher))
	{
		SwitcherSlot->SetSize(ESlateSizeRule::Fill);
	}

	auto MakePage = [&](FName Name) -> UVerticalBox*
	{
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), Name);
		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *(Name.ToString() + TEXT("_Content")));
		Scroll->AddChild(Content);
		TabSwitcher->AddChild(Scroll);
		return Content;
	};

	auto AddRow = [&](UVerticalBox* Page, FName BaseName, const FText& Label, UWidget* Control)
	{
		if (UVerticalBoxSlot* Slot = Page->AddChildToVerticalBox(JinzzaUI::MakeLabeledRow(WidgetTree, *(BaseName.ToString() + TEXT("_Row")), Label, Control)))
		{
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		}
	};

	// --- Graphics page (TabSwitcher index 0) ---
	UVerticalBox* GraphicsPage = MakePage(TEXT("GraphicsPage"));
	WindowModeCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("WindowModeCombo"));
	AddRow(GraphicsPage, TEXT("WindowMode"), FText::FromString(TEXT("Window Mode")), WindowModeCombo);
	ResolutionCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ResolutionCombo"));
	AddRow(GraphicsPage, TEXT("Resolution"), FText::FromString(TEXT("Resolution")), ResolutionCombo);
	VSyncCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("VSyncCheckBox"));
	AddRow(GraphicsPage, TEXT("VSync"), FText::FromString(TEXT("VSync")), VSyncCheckBox);
	FrameRateLimitSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("FrameRateLimitSpinBox"));
	AddRow(GraphicsPage, TEXT("FrameRateLimit"), FText::FromString(TEXT("Frame Rate Limit")), FrameRateLimitSpinBox);
	OverallQualityCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("OverallQualityCombo"));
	AddRow(GraphicsPage, TEXT("OverallQuality"), FText::FromString(TEXT("Overall Quality")), OverallQualityCombo);
	ViewDistanceSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("ViewDistanceSpinBox"));
	AddRow(GraphicsPage, TEXT("ViewDistance"), FText::FromString(TEXT("View Distance")), ViewDistanceSpinBox);
	ShadowSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("ShadowSpinBox"));
	AddRow(GraphicsPage, TEXT("Shadow"), FText::FromString(TEXT("Shadows")), ShadowSpinBox);
	GlobalIlluminationSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("GlobalIlluminationSpinBox"));
	AddRow(GraphicsPage, TEXT("GlobalIllumination"), FText::FromString(TEXT("Global Illumination")), GlobalIlluminationSpinBox);
	ReflectionSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("ReflectionSpinBox"));
	AddRow(GraphicsPage, TEXT("Reflection"), FText::FromString(TEXT("Reflections")), ReflectionSpinBox);
	AntiAliasingSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("AntiAliasingSpinBox"));
	AddRow(GraphicsPage, TEXT("AntiAliasing"), FText::FromString(TEXT("Anti-Aliasing")), AntiAliasingSpinBox);
	TextureSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("TextureSpinBox"));
	AddRow(GraphicsPage, TEXT("Texture"), FText::FromString(TEXT("Textures")), TextureSpinBox);
	EffectsSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("EffectsSpinBox"));
	AddRow(GraphicsPage, TEXT("Effects"), FText::FromString(TEXT("Effects")), EffectsSpinBox);
	FoliageSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("FoliageSpinBox"));
	AddRow(GraphicsPage, TEXT("Foliage"), FText::FromString(TEXT("Foliage")), FoliageSpinBox);
	ShadingSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("ShadingSpinBox"));
	AddRow(GraphicsPage, TEXT("Shading"), FText::FromString(TEXT("Shading")), ShadingSpinBox);

	// --- Audio page (index 1) ---
	UVerticalBox* AudioPage = MakePage(TEXT("AudioPage"));
	MasterVolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MasterVolumeSlider"));
	AddRow(AudioPage, TEXT("MasterVolume"), FText::FromString(TEXT("Master Volume")), MasterVolumeSlider);
	MusicVolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MusicVolumeSlider"));
	AddRow(AudioPage, TEXT("MusicVolume"), FText::FromString(TEXT("Music Volume")), MusicVolumeSlider);
	SFXVolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("SFXVolumeSlider"));
	AddRow(AudioPage, TEXT("SFXVolume"), FText::FromString(TEXT("SFX Volume")), SFXVolumeSlider);
	VoiceVolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("VoiceVolumeSlider"));
	AddRow(AudioPage, TEXT("VoiceVolume"), FText::FromString(TEXT("Voice Volume")), VoiceVolumeSlider);
	MicInputModeCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MicInputModeCombo"));
	AddRow(AudioPage, TEXT("MicInputMode"), FText::FromString(TEXT("Mic Input Mode")), MicInputModeCombo);
	MicDeviceCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MicDeviceCombo"));
	AddRow(AudioPage, TEXT("MicDevice"), FText::FromString(TEXT("Mic Device")), MicDeviceCombo);

	// --- Controls page (index 2) ---
	UVerticalBox* ControlsPage = MakePage(TEXT("ControlsPage"));
	MouseSensitivitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MouseSensitivitySlider"));
	AddRow(ControlsPage, TEXT("MouseSensitivity"), FText::FromString(TEXT("Mouse Sensitivity")), MouseSensitivitySlider);
	InvertYCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("InvertYCheckBox"));
	AddRow(ControlsPage, TEXT("InvertY"), FText::FromString(TEXT("Invert Y")), InvertYCheckBox);

	JumpRebindButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("JumpRebindButton"), FText::FromString(TEXT("Rebind")), 16.f);
	JumpRebindLabel = JinzzaUI::MakeBodyText(WidgetTree, TEXT("JumpRebindLabel"), FText::FromString(TEXT("Default")));
	AddRow(ControlsPage, TEXT("Jump"), FText::FromString(TEXT("Jump")), MakeRebindControl(WidgetTree, JumpRebindButton, JumpRebindLabel));

	SprintRebindButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("SprintRebindButton"), FText::FromString(TEXT("Rebind")), 16.f);
	SprintRebindLabel = JinzzaUI::MakeBodyText(WidgetTree, TEXT("SprintRebindLabel"), FText::FromString(TEXT("Default")));
	AddRow(ControlsPage, TEXT("Sprint"), FText::FromString(TEXT("Sprint")), MakeRebindControl(WidgetTree, SprintRebindButton, SprintRebindLabel));

	// --- Gameplay page (index 3) ---
	UVerticalBox* GameplayPage = MakePage(TEXT("GameplayPage"));
	SubtitlesCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("SubtitlesCheckBox"));
	AddRow(GameplayPage, TEXT("Subtitles"), FText::FromString(TEXT("Subtitles")), SubtitlesCheckBox);
	ColorblindModeCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ColorblindModeCombo"));
	AddRow(GameplayPage, TEXT("ColorblindMode"), FText::FromString(TEXT("Colorblind Mode")), ColorblindModeCombo);
	ColorblindStrengthSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("ColorblindStrengthSlider"));
	AddRow(GameplayPage, TEXT("ColorblindStrength"), FText::FromString(TEXT("Colorblind Strength")), ColorblindStrengthSlider);

	// --- Bottom-right Apply/Back row ---
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	if (UVerticalBoxSlot* ButtonRowSlot = ContentColumn->AddChildToVerticalBox(ButtonRow))
	{
		ButtonRowSlot->SetHorizontalAlignment(HAlign_Right);
		ButtonRowSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}

	BackButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("BackButton"), FText::FromString(TEXT("Back")));
	if (UHorizontalBoxSlot* BackSlot = ButtonRow->AddChildToHorizontalBox(BackButton))
	{
		BackSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	ApplyButton = JinzzaUI::MakePrimaryButton(WidgetTree, TEXT("ApplyButton"), FText::FromString(TEXT("Apply")));
	ButtonRow->AddChildToHorizontalBox(ApplyButton);
}

void UjinzzaSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

	if (GraphicsTabButton) GraphicsTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabGraphicsClicked);
	if (AudioTabButton) AudioTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabAudioClicked);
	if (ControlsTabButton) ControlsTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabControlsClicked);
	if (GameplayTabButton) GameplayTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabGameplayClicked);

	PopulateGraphicsPage();
	PopulateAudioPage();
	PopulateControlsPage();
	PopulateGameplayPage();

	SetActiveTab(Tab_Graphics);

	if (ApplyButton) ApplyButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnApplyClicked);
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnBackClicked);

	if (JumpRebindButton) JumpRebindButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindJumpClicked);
	if (SprintRebindButton) SprintRebindButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindSprintClicked);
}

void UjinzzaSettingsWidget::PopulateGraphicsPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (WindowModeCombo)
	{
		WindowModeCombo->AddOption(TEXT("Fullscreen"));
		WindowModeCombo->AddOption(TEXT("Windowed Fullscreen"));
		WindowModeCombo->AddOption(TEXT("Windowed"));

		FString CurrentWindowMode = TEXT("Fullscreen");
		switch (Settings->GetFullscreenMode())
		{
		case EWindowMode::Fullscreen: CurrentWindowMode = TEXT("Fullscreen"); break;
		case EWindowMode::WindowedFullscreen: CurrentWindowMode = TEXT("Windowed Fullscreen"); break;
		case EWindowMode::Windowed: CurrentWindowMode = TEXT("Windowed"); break;
		default: break;
		}
		WindowModeCombo->SetSelectedOption(CurrentWindowMode);
	}

	if (ResolutionCombo)
	{
		UKismetSystemLibrary::GetConvenientWindowedResolutions(AvailableResolutions);
		const FIntPoint CurrentResolution = Settings->GetScreenResolution();
		if (!AvailableResolutions.Contains(CurrentResolution))
		{
			AvailableResolutions.Insert(CurrentResolution, 0);
		}
		for (const FIntPoint& Res : AvailableResolutions)
		{
			ResolutionCombo->AddOption(ResolutionToString(Res));
		}
		ResolutionCombo->SetSelectedOption(ResolutionToString(CurrentResolution));
	}

	if (VSyncCheckBox)
	{
		VSyncCheckBox->SetIsChecked(Settings->IsVSyncEnabled());
	}

	if (FrameRateLimitSpinBox)
	{
		FrameRateLimitSpinBox->SetMinValue(0.f);
		FrameRateLimitSpinBox->SetMaxValue(300.f);
		FrameRateLimitSpinBox->SetMinSliderValue(0.f);
		FrameRateLimitSpinBox->SetMaxSliderValue(300.f);
		FrameRateLimitSpinBox->SetDelta(1.f);
		FrameRateLimitSpinBox->SetValue(Settings->GetFrameRateLimit());
	}

	if (OverallQualityCombo)
	{
		const TArray<FString> QualityPresets = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") };
		for (const FString& Preset : QualityPresets)
		{
			OverallQualityCombo->AddOption(Preset);
		}
		const int32 OverallLevel = FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, QualityPresets.Num() - 1);
		OverallQualityCombo->SetSelectedOption(QualityPresets[OverallLevel]);
	}

	auto InitQualitySpinBox = [](USpinBox* SpinBox, int32 InitialValue)
	{
		if (!SpinBox)
		{
			return;
		}
		SpinBox->SetMinValue(0.f);
		SpinBox->SetMaxValue(4.f);
		SpinBox->SetMinSliderValue(0.f);
		SpinBox->SetMaxSliderValue(4.f);
		SpinBox->SetDelta(1.f);
		SpinBox->SetValue(static_cast<float>(InitialValue));
	};

	InitQualitySpinBox(ViewDistanceSpinBox, Settings->GetViewDistanceQuality());
	InitQualitySpinBox(ShadowSpinBox, Settings->GetShadowQuality());
	InitQualitySpinBox(GlobalIlluminationSpinBox, Settings->GetGlobalIlluminationQuality());
	InitQualitySpinBox(ReflectionSpinBox, Settings->GetReflectionQuality());
	InitQualitySpinBox(AntiAliasingSpinBox, Settings->GetAntiAliasingQuality());
	InitQualitySpinBox(TextureSpinBox, Settings->GetTextureQuality());
	InitQualitySpinBox(EffectsSpinBox, Settings->GetVisualEffectQuality());
	InitQualitySpinBox(FoliageSpinBox, Settings->GetFoliageQuality());
	InitQualitySpinBox(ShadingSpinBox, Settings->GetShadingQuality());
}

void UjinzzaSettingsWidget::PopulateAudioPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetMinValue(0.f);
		MasterVolumeSlider->SetMaxValue(1.f);
		MasterVolumeSlider->SetValue(Settings->GetMasterVolume());
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetMinValue(0.f);
		MusicVolumeSlider->SetMaxValue(1.f);
		MusicVolumeSlider->SetValue(Settings->GetMusicVolume());
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->SetMinValue(0.f);
		SFXVolumeSlider->SetMaxValue(1.f);
		SFXVolumeSlider->SetValue(Settings->GetSFXVolume());
	}
	if (VoiceVolumeSlider)
	{
		VoiceVolumeSlider->SetMinValue(0.f);
		VoiceVolumeSlider->SetMaxValue(1.f);
		VoiceVolumeSlider->SetValue(Settings->GetVoiceVolume());
	}

	if (MicInputModeCombo)
	{
		MicInputModeCombo->AddOption(TEXT("Push to Talk"));
		MicInputModeCombo->AddOption(TEXT("Open Mic"));
		const TArray<FString> MicModes = { TEXT("Push to Talk"), TEXT("Open Mic") };
		MicInputModeCombo->SetSelectedOption(MicModes[static_cast<int32>(Settings->GetMicInputMode())]);
	}

	if (MicDeviceCombo)
	{
		AvailableMicDeviceIds = { FString() };
		MicDeviceCombo->AddOption(TEXT("System Default"));
		MicDeviceCombo->SetSelectedOption(TEXT("System Default"));

		// Device list is fetched async; OnAudioInputDevicesObtained repopulates the combo (and
		// restores the saved selection) once the platform responds, usually within a frame or two.
		FOnAudioInputDevicesObtained DevicesObtained;
		DevicesObtained.BindDynamic(this, &UjinzzaSettingsWidget::OnAudioInputDevicesObtained);
		UAudioCaptureBlueprintLibrary::GetAvailableAudioInputDevices(this, DevicesObtained);
	}
}

void UjinzzaSettingsWidget::OnAudioInputDevicesObtained(const TArray<FAudioInputDeviceInfo>& AvailableDevices)
{
	if (!MicDeviceCombo)
	{
		return;
	}

	const UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	const FString SavedDeviceId = Settings ? Settings->GetMicDeviceId() : FString();

	MicDeviceCombo->ClearOptions();
	MicDeviceCombo->AddOption(TEXT("System Default"));
	AvailableMicDeviceIds = { FString() };

	for (const FAudioInputDeviceInfo& Device : AvailableDevices)
	{
		AvailableMicDeviceIds.Add(Device.DeviceId);
		MicDeviceCombo->AddOption(Device.DeviceName);
	}

	const int32 SavedIndex = AvailableMicDeviceIds.IndexOfByKey(SavedDeviceId);
	MicDeviceCombo->SetSelectedIndex(SavedIndex != INDEX_NONE ? SavedIndex : 0);
}

void UjinzzaSettingsWidget::PopulateControlsPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->SetMinValue(0.1f);
		MouseSensitivitySlider->SetMaxValue(3.f);
		MouseSensitivitySlider->SetValue(Settings->GetMouseSensitivity());
	}
	if (InvertYCheckBox)
	{
		InvertYCheckBox->SetIsChecked(Settings->GetInvertYAxis());
	}

	RebindLabels.Reset();
	RebindLabels.Add(TEXT("IA_Jump"), JumpRebindLabel);
	RebindLabels.Add(TEXT("IA_Sprint"), SprintRebindLabel);

	RefreshRebindButtonLabel(TEXT("IA_Jump"));
	RefreshRebindButtonLabel(TEXT("IA_Sprint"));
}

void UjinzzaSettingsWidget::PopulateGameplayPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (SubtitlesCheckBox)
	{
		SubtitlesCheckBox->SetIsChecked(Settings->GetSubtitlesEnabled());
	}

	if (ColorblindModeCombo)
	{
		const TArray<FString> ColorblindOptions = { TEXT("Off"), TEXT("Deuteranope"), TEXT("Protanope"), TEXT("Tritanope") };
		for (const FString& Option : ColorblindOptions)
		{
			ColorblindModeCombo->AddOption(Option);
		}
		ColorblindModeCombo->SetSelectedOption(ColorblindOptions[static_cast<int32>(Settings->GetColorblindMode())]);
	}

	if (ColorblindStrengthSlider)
	{
		ColorblindStrengthSlider->SetMinValue(0.f);
		ColorblindStrengthSlider->SetMaxValue(1.f);
		ColorblindStrengthSlider->SetValue(Settings->GetColorblindStrength());
	}
}

void UjinzzaSettingsWidget::StartRebind(FName ActionName)
{
	bWaitingForRebind = true;
	PendingRebindAction = ActionName;

	if (TObjectPtr<UTextBlock>* Label = RebindLabels.Find(ActionName))
	{
		if (*Label)
		{
			(*Label)->SetText(FText::FromString(TEXT("Press a key...")));
		}
	}

	SetKeyboardFocus();
}

void UjinzzaSettingsWidget::RefreshRebindButtonLabel(FName ActionName)
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (TObjectPtr<UTextBlock>* Label = RebindLabels.Find(ActionName))
	{
		if (*Label)
		{
			const FKey Key = Settings->GetKeyRebind(ActionName);
			(*Label)->SetText(FText::FromString(Key.IsValid() ? Key.GetDisplayName().ToString() : TEXT("Default")));
		}
	}
}

FReply UjinzzaSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bWaitingForRebind)
	{
		if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
		{
			Settings->SetKeyRebind(PendingRebindAction, InKeyEvent.GetKey());
		}

		RefreshRebindButtonLabel(PendingRebindAction);
		bWaitingForRebind = false;
		PendingRebindAction = NAME_None;
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UjinzzaSettingsWidget::OnRebindJumpClicked() { StartRebind(TEXT("IA_Jump")); }
void UjinzzaSettingsWidget::OnRebindSprintClicked() { StartRebind(TEXT("IA_Sprint")); }

void UjinzzaSettingsWidget::SetActiveTab(int32 TabIndex)
{
	if (TabSwitcher)
	{
		TabSwitcher->SetActiveWidgetIndex(TabIndex);
	}

	UWidget* Accents[] = { GraphicsTabAccent, AudioTabAccent, ControlsTabAccent, GameplayTabAccent };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Accents); ++Index)
	{
		if (Accents[Index])
		{
			Accents[Index]->SetVisibility(Index == TabIndex ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
	}
}

void UjinzzaSettingsWidget::OnTabGraphicsClicked() { SetActiveTab(Tab_Graphics); }
void UjinzzaSettingsWidget::OnTabAudioClicked() { SetActiveTab(Tab_Audio); }
void UjinzzaSettingsWidget::OnTabControlsClicked() { SetActiveTab(Tab_Controls); }
void UjinzzaSettingsWidget::OnTabGameplayClicked() { SetActiveTab(Tab_Gameplay); }

void UjinzzaSettingsWidget::OnBackClicked()
{
	OnBackRequested.Broadcast();
}

void UjinzzaSettingsWidget::OnApplyClicked()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	// Graphics
	if (WindowModeCombo)
	{
		const FString Selection = WindowModeCombo->GetSelectedOption();
		EWindowMode::Type Mode = EWindowMode::Fullscreen;
		if (Selection == TEXT("Windowed Fullscreen")) Mode = EWindowMode::WindowedFullscreen;
		else if (Selection == TEXT("Windowed")) Mode = EWindowMode::Windowed;
		Settings->SetFullscreenMode(Mode);
	}
	if (ResolutionCombo)
	{
		const int32 Index = ResolutionCombo->GetSelectedIndex();
		if (AvailableResolutions.IsValidIndex(Index))
		{
			Settings->SetScreenResolution(AvailableResolutions[Index]);
		}
	}
	if (VSyncCheckBox) Settings->SetVSyncEnabled(VSyncCheckBox->IsChecked());
	if (FrameRateLimitSpinBox) Settings->SetFrameRateLimit(FrameRateLimitSpinBox->GetValue());
	if (OverallQualityCombo) Settings->SetOverallScalabilityLevel(OverallQualityCombo->GetSelectedIndex());
	if (ViewDistanceSpinBox) Settings->SetViewDistanceQuality(FMath::RoundToInt(ViewDistanceSpinBox->GetValue()));
	if (ShadowSpinBox) Settings->SetShadowQuality(FMath::RoundToInt(ShadowSpinBox->GetValue()));
	if (GlobalIlluminationSpinBox) Settings->SetGlobalIlluminationQuality(FMath::RoundToInt(GlobalIlluminationSpinBox->GetValue()));
	if (ReflectionSpinBox) Settings->SetReflectionQuality(FMath::RoundToInt(ReflectionSpinBox->GetValue()));
	if (AntiAliasingSpinBox) Settings->SetAntiAliasingQuality(FMath::RoundToInt(AntiAliasingSpinBox->GetValue()));
	if (TextureSpinBox) Settings->SetTextureQuality(FMath::RoundToInt(TextureSpinBox->GetValue()));
	if (EffectsSpinBox) Settings->SetVisualEffectQuality(FMath::RoundToInt(EffectsSpinBox->GetValue()));
	if (FoliageSpinBox) Settings->SetFoliageQuality(FMath::RoundToInt(FoliageSpinBox->GetValue()));
	if (ShadingSpinBox) Settings->SetShadingQuality(FMath::RoundToInt(ShadingSpinBox->GetValue()));

	// Audio
	if (MasterVolumeSlider) Settings->SetMasterVolume(MasterVolumeSlider->GetValue());
	if (MusicVolumeSlider) Settings->SetMusicVolume(MusicVolumeSlider->GetValue());
	if (SFXVolumeSlider) Settings->SetSFXVolume(SFXVolumeSlider->GetValue());
	if (VoiceVolumeSlider) Settings->SetVoiceVolume(VoiceVolumeSlider->GetValue());
	if (MicInputModeCombo) Settings->SetMicInputMode(MicInputModeCombo->GetSelectedIndex() == 1 ? EJinzzaMicInputMode::OpenMic : EJinzzaMicInputMode::PushToTalk);
	if (MicDeviceCombo && AvailableMicDeviceIds.IsValidIndex(MicDeviceCombo->GetSelectedIndex()))
	{
		Settings->SetMicDeviceId(AvailableMicDeviceIds[MicDeviceCombo->GetSelectedIndex()]);
	}

	// Controls
	if (MouseSensitivitySlider) Settings->SetMouseSensitivity(MouseSensitivitySlider->GetValue());
	if (InvertYCheckBox) Settings->SetInvertYAxis(InvertYCheckBox->IsChecked());

	// Gameplay
	if (SubtitlesCheckBox) Settings->SetSubtitlesEnabled(SubtitlesCheckBox->IsChecked());
	if (ColorblindModeCombo) Settings->SetColorblindMode(static_cast<EJinzzaColorblindMode>(ColorblindModeCombo->GetSelectedIndex()));
	if (ColorblindStrengthSlider) Settings->SetColorblindStrength(ColorblindStrengthSlider->GetValue());

	Settings->ApplySettings(false);
	Settings->SaveSettings();
}
